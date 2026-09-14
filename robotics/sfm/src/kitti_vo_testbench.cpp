#include "utils.h"
#include "sfm.h"
#include "tracker.h"
// #include <tclap/CmdLine.h>

#include <boost/program_options/options_description.hpp>
#include <boost/program_options/variables_map.hpp>
#include <boost/program_options.hpp>

namespace po = boost::program_options;
namespace fs = std::filesystem;

int main(int argc, char* argv[])
{
  po::options_description desc("Allowed options");
  desc.add_options()("help", "produce help message")("in", po::value<std::string>(), "path to kitti dataset folder")(
      "out", po::value<std::string>(), "path to results folder")("last_frame", po::value<int>()->default_value(200),
                                                                 "last frame of kitty processing")  // NOLINT
      ("first_frame", po::value<int>()->default_value(0), "first frame of kitty processing")(
          "sequence,s", po::value<std::string>()->default_value("03"), "name of kitti scene 03 for example")(
          "sfm,a", po::value<std::string>()->default_value("sfm"), "sfm algo [sfm|simple]")(
          "config,c", po::value<std::string>()->default_value("../../../../cpp/sfm/config/sfm.yaml"),
          "path to config file")("verbose,v", po::bool_switch()->default_value(false), "verbose mode");

  po::variables_map vm;
  po::store(po::parse_command_line(argc, argv, desc), vm);
  po::notify(vm);

  if (vm.count("help")) {
    std::cout << desc << "\n";
    return 1;
  }
  SFM::Config config;

  config.FEATURES_NUMBER = 2000;
  config.startFrame = vm["first_frame"].as<int>();  // vm["in"].as<std::string>()
  config.endFrame = vm["last_frame"].as<int>();
  config.verbose = vm["verbose"].as<bool>();

  std::cout << "3" << std::endl;
  fs::path sequencesPath = fs::path(vm["in"].as<std::string>()) / fs::path("sequences");
  std::cout << "4" << std::endl;
  fs::path sequencePath = sequencesPath / fs::path(vm["sequence"].as<std::string>());

  fs::path imagesPath = sequencePath / fs::path("image_0");
  for (const auto& path : fs::directory_iterator(imagesPath)) {
    config.paths.push_back(path.path());
  }
  std::sort(config.paths.begin(), config.paths.end());

  const auto first = cv::imread(config.paths[config.startFrame].string());

  fs::path calibPath = sequencePath / fs::path("calib.txt");
  config.calibration = getKittiCalibration(calibPath.string());
  config.calibration.intrinsic.width = first.size().width;
  config.calibration.intrinsic.height = first.size().height;

  fs::path gtPath =
      fs::path(vm["in"].as<std::string>()) / fs::path("poses") / fs::path(vm["sequence"].as<std::string>());
  gtPath.replace_extension(".txt");
  config.gt = parseGTFile(gtPath.string());

  auto odom = std::make_unique<Odometry>(config.gt[config.startFrame]);

  auto debug = std::make_shared<Debug>(config.calibration.extrinsic);

  std::unique_ptr<IRelativePoseProvider> relativeProvider =
      std::make_unique<RelativePoseProviderOpenCV>(config.calibration.intrinsic);
  // std::unique_ptr<IRelativePoseProvider> relativeProvider = std::make_unique<RelativePoseProviderOpenMVG>(intrinsic);

  auto monoVO = std::make_unique<TrackerOpenCV>(config.calibration.intrinsic, config.FEATURES_NUMBER);

  auto pipeline = std::make_shared<Mapping>(config.calibration.intrinsic);

  auto sfm =
      std::make_unique<SFM>(config, std::move(relativeProvider), std::move(monoVO), std::move(odom), debug, pipeline);
  sfm->run();

  fs::path trajectoryImgPath = fs::path(vm["out"].as<std::string>()) / fs::path("trajectory.png");
  debug->saveImg(trajectoryImgPath);

  fs::path sceneBeforePath = fs::path(vm["out"].as<std::string>()) / fs::path("scene.ply");
  // pipeline->adjust();
  pipeline->save(sceneBeforePath);
}
