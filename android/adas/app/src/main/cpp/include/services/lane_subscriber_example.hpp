// Example C++ subscriber for lanes published by ai.flow.adas.vision.LanePublisher
// Build into adas_app or a standalone tool; requires cppzmq + nlohmann_json (or parse JSON yourself).
//
//   zmq::context_t ctx{1};
//   zmq::socket_t sub{ctx, zmq::socket_type::sub};
//   sub.connect("tcp://127.0.0.1:5599");
//   sub.set(zmq::sockopt::subscribe, "lanes");
//
//   while (true) {
//     zmq::message_t topic, body;
//     auto r1 = sub.recv(topic, zmq::recv_flags::none);
//     auto r2 = sub.recv(body, zmq::recv_flags::none);
//     std::string json(static_cast<char*>(body.data()), body.size());
//     // json fields: t, frameId, x[33], lanes[4].{y[33],prob}, edges[2].{y[33]}
//     // units: meters, openpilot ego frame (x forward, y left)
//   }

#pragma once
// Header-only documentation placeholder — see comment above.
