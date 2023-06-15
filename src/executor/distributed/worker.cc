/**
MIT License

Copyright (c) 2020 lemonviv

    Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
SOFTWARE.
*/

//
// Created by on 11/08/21.
//

#include "falcon/distributed/worker.h"
#include <falcon/network/Comm.hpp>
#include <falcon/utils/base64.h>
#include <falcon/utils/logger/logger.h>
#include <falcon/utils/pb_converter/network_converter.h>
#include <glog/logging.h>

Worker::Worker(const std::string &ps_network_config_pb_str, int m_worker_id) {
  worker_id = m_worker_id;
  std::vector<std::string> worker_ips;
  std::vector<int> worker_ports;
  std::vector<std::string> ps_ips;
  std::vector<int> ps_ports;
  // 0. decode the network base64 string to pb string
  // vector<BYTE> ps_network_file_byte = base64_decode(ps_network_file);
  // std::string ps_network_pb_string(ps_network_file_byte.begin(),
  // ps_network_file_byte.end());
  log_info("begin to deserialize the ps network configs");
  // 1. read network config proto from coordinator
  deserialize_ps_network_configs(worker_ips, worker_ports, ps_ips, ps_ports,
                                 ps_network_config_pb_str);
  log_info("Establish network communications with parameter server, current "
           "worker id = " +
           to_string(worker_id));

#ifdef DEBUG
  for (int i = 0; i < worker_ips.size(); i++) {
    log_info("worker_ips[" + std::to_string(i) + "] = " + worker_ips[i]);
    log_info("worker_ports[" + std::to_string(i) +
             "] = " + std::to_string(worker_ports[i]));
    log_info("ps_ips[" + std::to_string(i) + "] = " + ps_ips[i]);
    log_info("ps_ports[" + std::to_string(i) +
             "] = " + std::to_string(ps_ports[i]));
  }
#endif

  // 2. establish communication connections between worker and parameter server
  // current party instance is created at worker, only build channel with ps
  SocketPartyData me_worker, other_ps;
  me_worker =
      SocketPartyData(boost_ip::address::from_string(worker_ips[worker_id - 1]),
                      worker_ports[worker_id - 1]);
  other_ps =
      SocketPartyData(boost_ip::address::from_string(ps_ips[worker_id - 1]),
                      ps_ports[worker_id - 1]);
  ps_channel = make_shared<CommPartyTCPSynced>(io_service, me_worker, other_ps);

  // connect to the other party and add channel
  ps_channel->join(500, 5000);
  log_info("Communication channel established with ps " +
           ps_ips[worker_id - 1] + ", port is " +
           std::to_string(ps_ports[worker_id - 1]));
}

Worker::~Worker() {
  io_service.stop();
  ps_channel = nullptr;
};

void Worker::send_long_message_to_ps(std::string message) const {
  ps_channel->writeWithSize(std::move(message));
}

void Worker::recv_long_message_from_ps(std::string &message) const {
  vector<byte> recv_message;
  ps_channel->readWithSizeIntoVector(recv_message);
  const byte *uc = &(recv_message[0]);
  string recv_message_str(reinterpret_cast<char const *>(uc),
                          recv_message.size());
  message = recv_message_str;
}
