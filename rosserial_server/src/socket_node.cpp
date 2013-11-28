/**
 *
 *  \file
 *  \brief      Main entry point for the socket server node.
 *  \author     Mike Purvis <mpurvis@clearpathrobotics.com>
 *  \copyright  Copyright (c) 2013, Clearpath Robotics, Inc.
 * 
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions are met:
 *     * Redistributions of source code must retain the above copyright
 *       notice, this list of conditions and the following disclaimer.
 *     * Redistributions in binary form must reproduce the above copyright
 *       notice, this list of conditions and the following disclaimer in the
 *       documentation and/or other materials provided with the distribution.
 *     * Neither the name of Clearpath Robotics, Inc. nor the
 *       names of its contributors may be used to endorse or promote products
 *       derived from this software without specific prior written permission.
 * 
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS" AND
 * ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE
 * DISCLAIMED. IN NO EVENT SHALL CLEARPATH ROBOTICS, INC. BE LIABLE FOR ANY
 * DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES
 * (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 * LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND
 * ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 * 
 * Please send comments, questions, or patches to code@clearpathrobotics.com 
 *
 */

#include <iostream>
#include <boost/bind.hpp>
#include <boost/asio.hpp>

#include <ros/ros.h>

#include "Session.h"
#include "AsyncOkPoll.h"


using boost::asio::ip::tcp;


template<typename Session>
class TcpServer
{
public:
  TcpServer(boost::asio::io_service& io_service, short port)
    : io_service_(io_service),
      acceptor_(io_service, tcp::endpoint(tcp::v4(), port))
  {
    start_accept();
  }

private:
  void start_accept()
  {
    Session* new_session = new Session(io_service_);
    acceptor_.async_accept(new_session->socket(),
        boost::bind(&TcpServer::handle_accept, this, new_session,
          boost::asio::placeholders::error));
  }

  void handle_accept(Session* new_session,
      const boost::system::error_code& error)
  {
    if (!error)
    {
      new_session->start();
    }
    else
    {
      delete new_session;
    }

    start_accept();
  }

  boost::asio::io_service& io_service_;
  tcp::acceptor acceptor_;
};


int main(int argc, char* argv[])
{
  boost::asio::io_service io_service;

  // Initialize ROS.
  ros::init(argc, argv, "rosserial_server_socket_node");
  ros::NodeHandle nh("~");

  // ROS background thread.
  ros::AsyncSpinner ros_spinner(1);
  ros_spinner.start();

  // Monitor ROS for shutdown, and stop the io_service accordingly.
  AsyncOkPoll ok_poll(io_service, boost::posix_time::milliseconds(500), ros::ok);

  // Start listening for rosserial TCP connections.
  int port;
  nh.param<int>("port", port, 11411);
  TcpServer< Session<tcp::socket> > s(io_service, port);
  std::cout << "Listening on port " << port << "\n";
  io_service.run();

  return 0;
}