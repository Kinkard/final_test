#include <unistd.h>
#include <iostream>
#include <string.h>

#include <array>

// for connection pull
#include <deque>
#include <pthread.h>
#include <mutex>
#include <condition_variable>

#include <boost/asio.hpp>
#include <boost/shared_ptr.hpp>
#include <fstream>

#include "mime_types.hpp"
#include "request_parser.h"
#include "helper.h"

using namespace boost::asio;
using boost::asio::ip::tcp;
using socket_ptr = boost::shared_ptr<tcp::socket>;

struct connection_pull;
using connection_pull_ptr = boost::shared_ptr<connection_pull>;

#define MAX_ARG 512
#define THREAD_COUNT 10

struct connection_pull
{
  std::deque<socket_ptr> sockets;
  std::mutex guard;
  std::condition_variable new_added;
};

void* connetion_handler(void *arg);

char directory[MAX_ARG];
int main(int argc, char **argv)
{
  char ip[MAX_ARG], port[MAX_ARG];
  bool daemon_mode = true;

  // get input args
  int c;
  while((c = getopt(argc, argv, "h:p:d:g")) != -1)
  {
    switch (c)
    {
      case 'h': strncpy(ip, optarg, MAX_ARG); break;

      case 'p': strncpy(port, optarg, MAX_ARG); break;

      case 'd': strncpy(directory, optarg, MAX_ARG); break;

      case 'g': daemon_mode = false; break;

    };
  }

  if(daemon_mode)
  {
    int pid = fork();
    if (pid)
      return 0;

    umask(0);
    setsid();

    close(STDIN_FILENO);
    close(STDOUT_FILENO);
    close(STDERR_FILENO);
  }

//  std::cout << "ip: " << ip << " port: " << port << " directory: " << directory << std::endl;
//  std::cout << "startup asio" << std::endl;
  // startup boost asio
  boost::asio::io_service io_service;
  tcp::endpoint endpoint(ip::address::from_string(ip), atoi(port));
  tcp::acceptor acceptor(io_service, endpoint);

  if(!acceptor.is_open())
    return 0;

//  std::cout << "startup threads" << std::endl;
  connection_pull *pull = new connection_pull;

  pthread_t t_pull[THREAD_COUNT];
  // create threads pull
//  std::cout << "start push!" << std::endl;
  for (size_t i = 0; i < THREAD_COUNT; ++i) {
//    std::cout << "thread #" << i << " created!" << std::endl;
    pthread_create(t_pull + i, NULL, connetion_handler, (void*)pull);
  }

//  std::cout << "startup socket accepting endless loop" << std::endl;
  // endless loop for socket accepting
  for(;;)
  {
    // make shared to share this ptr to other threads
    socket_ptr sock(new tcp::socket(io_service));

    // listen
    acceptor.accept(*sock);

//    std::cout << "conection fetched!" << std::endl;

    // push back to pull and notify all threads
    std::unique_lock<std::mutex> ul(pull->guard);
    pull->sockets.push_back(sock);
    pull->new_added.notify_one();
  }

  return 0;
}

void* connetion_handler(void *arg)
{
  connection_pull *pull = (connection_pull*)arg;
  // endless loop for socket handling
  for(;;)
  {
    socket_ptr sock;
    {
      std::unique_lock<std::mutex> ul(pull->guard);
      while (!pull->sockets.size()) // loop to avoid spurious wakeups
        pull->new_added.wait(ul);

      sock = pull->sockets.front();
      pull->sockets.pop_front();
    }


//    std::cout << "start processing connection" << std::endl;
    //do smth with socket
    std::array<char, 1024> data;

    size_t bytes_transferred = sock->read_some(buffer(data));
    request_parser request_parser_;
    request request_;
    reply rep;

    request_parser::result_type result = request_parser_.parse(
        request_, data.data(), data.data() + bytes_transferred);
//    switch (result)
//    {
//      case request_parser::good:
//        std::cout << "Good request" << std::endl;
//        break;
//      case request_parser::indeterminate:
//        std::cout << "Intermidiate request" << std::endl;
//        break;
//      case request_parser::bad:
//        std::cout << "Bad request" << std::endl;
//        break;
//    }


    if(result == request_parser::good)
    {
      std::string request_path;
      if (url_decode(request_.uri, request_path))
      {
        // for debug purpose
        bool empty = request_path.empty() || (request_path.length() == 1);
        bool no_start_with_slash = request_path[0] != '/';
        bool try_to_up = request_path.find("..") != std::string::npos;
        if (!empty && !no_start_with_slash && !try_to_up)
        {
          std::size_t last_slash_pos = request_path.find_last_of("/");
          std::size_t last_dot_pos = request_path.find_last_of(".");
          std::string extension;
          if (last_dot_pos != std::string::npos && last_dot_pos > last_slash_pos)
          {
            extension = request_path.substr(last_dot_pos + 1);
          }

          // Open the file to send back.
          std::string full_path = directory + request_path;
          std::ifstream is(full_path.c_str(), std::ios::in | std::ios::binary);
          if (is)
          {
            // Fill out the reply to be sent to the client.
            rep.status = reply::ok;
            char buf[512];
            while (is.read(buf, sizeof(buf)).gcount() > 0)
              rep.content.append(buf, is.gcount());
            rep.headers.resize(2);
            rep.headers[0].name = "Content-Length";
            rep.headers[0].value = std::to_string(rep.content.size());
            rep.headers[1].name = "Content-Type";
            rep.headers[1].value = extension_to_type(extension);
          }
          else
              rep = reply::stock_reply(reply::not_found);
        }
        else
          rep = reply::stock_reply(reply::not_found);

      }
      else
        rep = reply::stock_reply(reply::not_found);
    }
    else
      rep = reply::stock_reply(reply::not_found);

    sock->write_some(rep.to_buffers());

    sock->close();

//    std::cout << "processing finished!" << std::endl;
  }

  return arg;
}