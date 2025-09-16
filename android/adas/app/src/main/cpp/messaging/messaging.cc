#include "messaging.h"
#include "impl_zmq.h"

Context* Context::create() {
  return new ZMQContext();
}

SubSocket* SubSocket::create() {
  return new ZMQSubSocket();
}

SubSocket* SubSocket::create(Context * context, std::string endpoint, std::string address, bool conflate, bool check_endpoint) {
  ZMQSubSocket* s = new ZMQSubSocket();
  s->connect(context, endpoint, address, conflate, check_endpoint);
  return s;
}

PubSocket* PubSocket::create() {
  return new ZMQPubSocket();
}

PubSocket* PubSocket::create(Context * context, std::string endpoint, bool check_endpoint) {
  ZMQPubSocket* s = new ZMQPubSocket();
  s->connect(context, endpoint, check_endpoint);
  return s;
}

PubSocket* PubSocket::create(Context * context, std::string endpoint, int port, bool check_endpoint) {
  ZMQPubSocket* s = new ZMQPubSocket();
  s->connect(context, endpoint, check_endpoint);
  return s;
}

Poller* Poller::create() {
  return new ZMQPoller();
}

Poller* Poller::create(std::vector<SubSocket*> sockets) {
  ZMQPoller* p = new ZMQPoller();
  for (auto* s : sockets) {
    p->registerSocket(s);
  }
  return p;
}




