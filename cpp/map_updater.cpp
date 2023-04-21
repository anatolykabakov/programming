#include <pcl/point_cloud.h>
#include <pcl/kdtree/kdtree_flann.h>
#include <pcl/filters/crop_box.h>
#include <pcl/common/common.h>
#include <pcl/io/pcd_io.h>
#include <iostream>
#include <vector>
#include <ctime>
#include <unordered_map>

enum class CResult { TP, TN, FN, FP };

std::unordered_map<CResult, std::string> association = {{CResult::TP, "TP"},
                                                        {CResult::TN, "TN"},
                                                        {CResult::FP, "FP"},
                                                        {CResult::FN, "FN"}};

class MapUpdater {
public:
  MapUpdater(float radius_search_threshold, bool debug)
    : radius_search_threshold_(radius_search_threshold), debug_(debug) {
    if (debug) {
      debug_map_.reset(new pcl::PointCloud<pcl::PointXYZRGB>());
    }
    new_map_.reset(new pcl::PointCloud<pcl::PointXYZI>());
  }

  void set_input_cloud(pcl::PointCloud<pcl::PointXYZ>::Ptr map) { global_map_ = map; }

  void update(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud) {
    auto submap = crop_(cloud);
    m_map_.clear();
    m_scan_.clear();
    m_map_.resize(submap->points.size(), CResult::FP);
    m_scan_.resize(cloud->points.size(), CResult::FN);
    classify_(submap, cloud);
    update_map_(submap, cloud);

    if (debug_) {
      update_debug_map_(submap, cloud);
    }
  }

  pcl::PointCloud<pcl::PointXYZRGB>::Ptr get_debug_map() {
    debug_map_->height = 1;
    debug_map_->width = debug_map_->points.size();
    return debug_map_;
  }

  pcl::PointCloud<pcl::PointXYZI>::Ptr get_new_map() {
    new_map_->height = 1;
    new_map_->width = new_map_->points.size();
    return new_map_;
  }

private:
  pcl::PointCloud<pcl::PointXYZ>::Ptr global_map_;
  pcl::PointCloud<pcl::PointXYZI>::Ptr new_map_;
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr debug_map_;
  std::vector<CResult> m_map_;
  std::vector<CResult> m_scan_;
  float radius_search_threshold_;
  bool debug_;

private:
  pcl::PointCloud<pcl::PointXYZ>::Ptr crop_(pcl::PointCloud<pcl::PointXYZ>::Ptr cloud) {
    pcl::PointCloud<pcl::PointXYZ>::Ptr submap(new pcl::PointCloud<pcl::PointXYZ>());

    pcl::CropBox<pcl::PointXYZ> box_filter;
    pcl::PointXYZ min_pt, max_pt;
    pcl::getMinMax3D(*cloud, min_pt, max_pt);
    box_filter.setMin(Eigen::Vector4f(min_pt.x, min_pt.y, min_pt.z, 1.0));
    box_filter.setMax(Eigen::Vector4f(max_pt.x, max_pt.y, max_pt.z, 1.0));
    box_filter.setInputCloud(global_map_);
    box_filter.filter(*submap);

    return submap;
  }

  void classify_(pcl::PointCloud<pcl::PointXYZ>::Ptr submap, pcl::PointCloud<pcl::PointXYZ>::Ptr scan) {
    pcl::KdTreeFLANN<pcl::PointXYZ> kdtree_map;
    kdtree_map.setInputCloud(submap);

    pcl::KdTreeFLANN<pcl::PointXYZ> kdtree_scan;
    kdtree_scan.setInputCloud(scan);

    for (int i = 0; i < scan->points.size(); ++i) {
      auto& point = scan->points[i];
      std::vector<int> inds(1, -1);
      std::vector<float> dists(1, -1.0);
      int k = kdtree_map.radiusSearch(point, 1, inds, dists);
      if (k > 0 && dists[0] <= radius_search_threshold_) {
        m_scan_[i] = CResult::TP;
      }
    }

    for (int i = 0; i < submap->points.size(); ++i) {
      auto& point = submap->points[i];
      std::vector<int> inds(1, -1);
      std::vector<float> dists(1, -1.0);
      int k = kdtree_scan.radiusSearch(point, 1, inds, dists);
      if (k > 0 && dists[0] <= radius_search_threshold_) {
        m_map_[i] = CResult::TP;
      }
    }
  }

  void update_map_(pcl::PointCloud<pcl::PointXYZ>::Ptr submap, pcl::PointCloud<pcl::PointXYZ>::Ptr scan) {
    for (int i = 0; i < m_map_.size(); ++i) {
      if (m_map_[i] == CResult::TP) {
        pcl::PointXYZI tp;
        tp.x = submap->points[i].x;
        tp.y = submap->points[i].y;
        tp.z = submap->points[i].z;
        new_map_->points.push_back(tp);
      }
    }
    for (int i = 0; i < m_scan_.size(); ++i) {
      if (m_scan_[i] == CResult::FN) {
        pcl::PointXYZI fn;
        fn.x = scan->points[i].x;
        fn.y = scan->points[i].y;
        fn.z = scan->points[i].z;
        new_map_->points.push_back(fn);
      }
    }
  }

  void update_debug_map_(pcl::PointCloud<pcl::PointXYZ>::Ptr submap, pcl::PointCloud<pcl::PointXYZ>::Ptr scan) {
    for (int i = 0; i < m_map_.size(); ++i) {
      if (m_map_[i] == CResult::FP) {
        pcl::PointXYZRGB fp;
        fp.x = submap->points[i].x;
        fp.y = submap->points[i].y;
        fp.z = submap->points[i].z;
        fp.r = static_cast<unsigned char>(255);
        fp.g = static_cast<unsigned char>(0);
        fp.b = static_cast<unsigned char>(0);
        debug_map_->points.push_back(fp);
      }
      if (m_map_[i] == CResult::TP) {
        pcl::PointXYZRGB tp;
        tp.x = submap->points[i].x;
        tp.y = submap->points[i].y;
        tp.z = submap->points[i].z;
        tp.r = static_cast<unsigned char>(0);
        tp.g = static_cast<unsigned char>(255);
        tp.b = static_cast<unsigned char>(0);
        debug_map_->points.push_back(tp);
      }
    }
    for (int i = 0; i < m_scan_.size(); ++i) {
      if (m_scan_[i] == CResult::FN) {
        pcl::PointXYZRGB fn;
        fn.x = scan->points[i].x;
        fn.y = scan->points[i].y;
        fn.z = scan->points[i].z;
        fn.r = static_cast<unsigned char>(0);
        fn.g = static_cast<unsigned char>(0);
        fn.b = static_cast<unsigned char>(255);
        debug_map_->points.push_back(fn);
      }
    }
  }
};

pcl::PointCloud<pcl::PointXYZ>::Ptr foo(pcl::PointCloud<pcl::PointXYZ>::Ptr global_map,
                                        pcl::PointCloud<pcl::PointXYZ>::Ptr scan, float threshold = 1.0) {
  pcl::PointCloud<pcl::PointXYZ>::Ptr submap(new pcl::PointCloud<pcl::PointXYZ>());

  pcl::CropBox<pcl::PointXYZ> box_filter;
  pcl::PointXYZ min_pt, max_pt;
  pcl::getMinMax3D(*scan, min_pt, max_pt);
  box_filter.setMin(Eigen::Vector4f(min_pt.x, min_pt.y, min_pt.z, 1.0));
  box_filter.setMax(Eigen::Vector4f(max_pt.x, max_pt.y, max_pt.z, 1.0));
  box_filter.setInputCloud(global_map);
  box_filter.filter(*submap);

  pcl::KdTreeFLANN<pcl::PointXYZ> kdtree_map;
  kdtree_map.setInputCloud(submap);

  pcl::KdTreeFLANN<pcl::PointXYZ> kdtree_scan;
  kdtree_scan.setInputCloud(scan);

  std::vector<CResult> m_map(submap->points.size(), CResult::FP);
  std::vector<CResult> m_scan(scan->points.size(), CResult::FN);

  for (int i = 0; i < scan->points.size(); ++i) {
    auto& point = scan->points[i];
    std::vector<int> inds(1, -1);
    std::vector<float> dists(1, -1.0);
    int k = kdtree_map.radiusSearch(point, 1, inds, dists);
    if (k > 0 && dists[0] <= threshold) {
      m_scan[i] = CResult::TP;
    }
  }

  for (int i = 0; i < submap->points.size(); ++i) {
    auto& point = submap->points[i];
    std::vector<int> inds(1, -1);
    std::vector<float> dists(1, -1.0);
    int k = kdtree_scan.radiusSearch(point, 1, inds, dists);
    if (k > 0 && dists[0] <= threshold) {
      m_map[i] = CResult::TP;
    }
  }

  pcl::PointCloud<pcl::PointXYZ>::Ptr new_map(new pcl::PointCloud<pcl::PointXYZ>);
  for (int i = 0; i < submap->points.size(); ++i) {
    if (m_map[i] == CResult::TP) {
      new_map->points.push_back(submap->points[i]);
    }
  }

  for (int i = 0; i < scan->points.size(); ++i) {
    if (m_scan[i] == CResult::FN) {
      new_map->points.push_back(scan->points[i]);
    }
  }

  return new_map;
}

void testcase1() {
  std::cout << "testcase 1" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  map->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  map->points.push_back(pcl::PointXYZ(0.0, 10.0, 0.0));
  map->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  scan->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(0.0, 10.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  auto result = foo(map, scan);

  for (int i = 0; i < result->points.size(); ++i) {
    std::cout << result->points[i].x << " " << result->points[i].y << " " << result->points[i].z << std::endl;
  }
}

void testcase2() {
  std::cout << "testcase 2" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  map->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  map->points.push_back(pcl::PointXYZ(0.0, 10.0, 0.0));
  map->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  scan->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  auto result = foo(map, scan);

  for (int i = 0; i < result->points.size(); ++i) {
    std::cout << result->points[i].x << " " << result->points[i].y << " " << result->points[i].z << std::endl;
  }
}

void testcase3() {
  std::cout << "testcase 3" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  map->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  map->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  scan->points.push_back(pcl::PointXYZ(0.0, 0.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(0.0, 10.0, 0.0));
  scan->points.push_back(pcl::PointXYZ(10.0, 0.0, 0.0));

  auto result = foo(map, scan);

  for (int i = 0; i < result->points.size(); ++i) {
    std::cout << result->points[i].x << " " << result->points[i].y << " " << result->points[i].z << std::endl;
  }
}

void testcase4() {
  std::cout << "testcase 4" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::io::loadPCDFile("/home/devel/Workspace/git/programming/cpp/testdata/cropped.pcd", *map);
  pcl::io::loadPCDFile("/home/devel/Workspace/git/programming/cpp/testdata/cloud.pcd", *scan);
  auto result = foo(map, scan);

  pcl::io::savePCDFileBinary("/home/devel/Workspace/git/programming/cpp/testdata/testcase4.pcd", *result);
}

void testcase5() {
  std::cout << "testcase 4" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::io::loadPCDFile("/home/devel/Workspace/git/programming/cpp/testdata/cloud.pcd", *map);
  pcl::io::loadPCDFile("/home/devel/Workspace/git/programming/cpp/testdata/cropped.pcd", *scan);
  auto result = foo(map, scan);

  pcl::io::savePCDFileBinary("/home/devel/Workspace/git/programming/cpp/testdata/testcase5.pcd", *result);
}

void testcase6() {
  std::cout << "testcase 6" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::io::loadPCDFile("/home/devel/Workspace/git/programming/cpp/testdata/cloud.pcd", *map);
  pcl::io::loadPCDFile("/home/devel/Workspace/git/programming/cpp/testdata/cropped.pcd", *scan);
  auto result = foo(map, scan);

  MapUpdater update{1.0, true};
  update.set_input_cloud(map);
  update.update(scan);
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr debug = update.get_debug_map();
  pcl::PointCloud<pcl::PointXYZI>::Ptr new_map = update.get_new_map();

  pcl::io::savePCDFileBinary("/home/devel/Workspace/git/programming/cpp/testdata/testcase6_debug.pcd", *debug);
  pcl::io::savePCDFileBinary("/home/devel/Workspace/git/programming/cpp/testdata/testcase6_new_map.pcd", *new_map);
}

void testcase7() {
  std::cout << "testcase 6" << std::endl;
  pcl::PointCloud<pcl::PointXYZ>::Ptr map(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::PointCloud<pcl::PointXYZ>::Ptr scan(new pcl::PointCloud<pcl::PointXYZ>);
  pcl::io::loadPCDFile("/home/devel/Workspace/git/programming/cpp/testdata/cloud.pcd", *map);
  pcl::io::loadPCDFile("/home/devel/Workspace/git/programming/cpp/testdata/cropped.pcd", *scan);
  auto result = foo(map, scan);

  MapUpdater update{1.0, true};
  update.set_input_cloud(scan);
  update.update(map);
  pcl::PointCloud<pcl::PointXYZRGB>::Ptr debug = update.get_debug_map();
  pcl::PointCloud<pcl::PointXYZI>::Ptr new_map = update.get_new_map();

  pcl::io::savePCDFileBinary("/home/devel/Workspace/git/programming/cpp/testdata/testcase7_debug.pcd", *debug);
  pcl::io::savePCDFileBinary("/home/devel/Workspace/git/programming/cpp/testdata/testcase7_new_map.pcd", *new_map);
}

int main(int argc, char** argv) {
  testcase1();
  testcase2();
  testcase3();
  testcase4();
  testcase5();
  testcase6();
  testcase7();

  return 0;
}
