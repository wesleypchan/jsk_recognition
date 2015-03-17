// -*- mode: c++ -*-
/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2014, JSK Lab
 *  All rights reserved.
 *
 *  Redistribution and use in source and binary forms, with or without
 *  modification, are permitted provided that the following conditions
 *  are met:
 *
 *   * Redistributions of source code must retain the above copyright
 *     notice, this list of conditions and the following disclaimer.
 *   * Redistributions in binary form must reproduce the above
 *     copyright notice, this list of conditions and the following
 *     disclaimer in the documentation and/o2r other materials provided
 *     with the distribution.
 *   * Neither the name of the JSK Lab nor the names of its
 *     contributors may be used to endorse or promote products derived
 *     from this software without specific prior written permission.
 *
 *  THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 *  "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
 *  LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS
 *  FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
 *  COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT,
 *  INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING,
 *  BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES;
 *  LOSS OF USE, DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER
 *  CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
 *  LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN
 *  ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
 *  POSSIBILITY OF SUCH DAMAGE.
 *********************************************************************/

#ifndef JSK_PCL_ROS_ENVIRONMENT_PLANE_MODELING_H_
#define JSK_PCL_ROS_ENVIRONMENT_PLANE_MODELING_H_

#include <pcl_ros/pcl_nodelet.h>

#include <pcl/kdtree/kdtree_flann.h>
#include <message_filters/subscriber.h>
#include <message_filters/time_synchronizer.h>
#include <message_filters/synchronizer.h>
#include <dynamic_reconfigure/server.h>

#include <jsk_recognition_msgs/PolygonArray.h>
#include <jsk_recognition_msgs/ModelCoefficientsArray.h>
#include <jsk_recognition_msgs/ClusterPointIndices.h>
#include <sensor_msgs/PointCloud2.h>
#include <jsk_pcl_ros/EnvironmentLock.h>
#include <jsk_pcl_ros/PolygonOnEnvironment.h>

#include <jsk_pcl_ros/pcl_conversion_util.h>
#include <jsk_pcl_ros/EnvironmentPlaneModelingConfig.h>

#include <diagnostic_updater/diagnostic_updater.h>
#include <diagnostic_updater/publisher.h>

#include <std_srvs/Empty.h>

#include <jsk_topic_tools/time_accumulator.h>

#include "jsk_pcl_ros/pcl_util.h"
#include <jsk_recognition_msgs/SimpleOccupancyGridArray.h>
#include <jsk_topic_tools/diagnostic_nodelet.h>
#include "jsk_pcl_ros/geo_util.h"

namespace jsk_pcl_ros
{

  // Helper classes

  /**
   * @brief
   * Nodelet implementation of jsk_pcl/EnvironmentPlaneModeling
   */
  class EnvironmentPlaneModeling: public jsk_topic_tools::DiagnosticNodelet
  {
  public:
    typedef EnvironmentPlaneModelingConfig Config;
    
    typedef message_filters::sync_policies::ExactTime<
      sensor_msgs::PointCloud2,
      jsk_recognition_msgs::PolygonArray,
      jsk_recognition_msgs::ModelCoefficientsArray,
      jsk_recognition_msgs::ClusterPointIndices > SyncPolicy;
    EnvironmentPlaneModeling(): DiagnosticNodelet("EnvironmentPlaneModeling") {}
  protected:
    virtual void onInit();

    /**
     * @brief
     * subscription callback function of jsk_topic_tools::DiagnosticNodelet.
     * This method is empty method because EnvironmentPlaneModeling needs to always run
     */
    virtual void subscribe() {}

    /**
     * @brief
     * unsubscription callback function of jsk_topic_tools::DiagnosticNodelet.
     * This method is empty method because EnvironmentPlaneModeling needs to always run
     */
    virtual void unsubscribe() {}

    /**
     * @brief
     * main callback function
     */
    virtual void inputCallback(
      const sensor_msgs::PointCloud2::ConstPtr& cloud_msg,
      const jsk_recognition_msgs::PolygonArray::ConstPtr& polygon_msg,
      const jsk_recognition_msgs::ModelCoefficientsArray::ConstPtr& coefficients_msg,
      const jsk_recognition_msgs::ClusterPointIndices::ConstPtr& indices_msg);

    virtual void printInputData(
      const sensor_msgs::PointCloud2::ConstPtr& cloud_msg,
      const jsk_recognition_msgs::PolygonArray::ConstPtr& polygon_msg,
      const jsk_recognition_msgs::ModelCoefficientsArray::ConstPtr& coefficients_msg,
      const jsk_recognition_msgs::ClusterPointIndices::ConstPtr& indices_msg);


    virtual bool isValidFrameIds(
      const sensor_msgs::PointCloud2::ConstPtr& cloud_msg,
      const jsk_recognition_msgs::PolygonArray::ConstPtr& polygon_msg,
      const jsk_recognition_msgs::ModelCoefficientsArray::ConstPtr& coefficients_msg,
      const jsk_recognition_msgs::ClusterPointIndices::ConstPtr& indices_msg);

    virtual std::vector<ConvexPolygon::Ptr> convertToConvexPolygons(
      const pcl::PointCloud<pcl::PointNormal>::Ptr& cloud,
      const jsk_recognition_msgs::ClusterPointIndices::ConstPtr& indices_msg,
      const jsk_recognition_msgs::ModelCoefficientsArray::ConstPtr& coefficients_msg);

    virtual void configCallback(Config &config, uint32_t level);

    /**
     * @brief
     * Publish array of ConvexPolygon::Ptr by using specified publisher
     */
    virtual void publishConvexPolygons(
      ros::Publisher& pub,
      const std_msgs::Header& header,
      std::vector<ConvexPolygon::Ptr>& convexes);
    
    /**
     * @brief
     * Publish array of GridPlane::Ptr by using specified publisher
     */
    virtual void publishGridMaps(
      ros::Publisher& pub,
      const std_msgs::Header& header,
      std::vector<GridPlane::Ptr>& grids);
    
    /**
     * @brief
     * Magnify ConvexPolygons according to maginify_distance_ parameter.
     */
    virtual std::vector<ConvexPolygon::Ptr> magnifyConvexes(
      std::vector<ConvexPolygon::Ptr>& convexes);

    /**
     * @brief
     * make GridPlane from ConvexPolygon and PointCloud
     */
    virtual std::vector<GridPlane::Ptr> buildGridPlanes(
      const pcl::PointCloud<pcl::PointNormal>::Ptr& cloud,
      std::vector<ConvexPolygon::Ptr> convexes);
    
    boost::mutex mutex_;
    boost::shared_ptr<message_filters::Synchronizer<SyncPolicy> > sync_;
    message_filters::Subscriber<sensor_msgs::PointCloud2> sub_cloud_;
    message_filters::Subscriber<jsk_recognition_msgs::ClusterPointIndices> sub_indices_;
    message_filters::Subscriber<jsk_recognition_msgs::PolygonArray> sub_polygons_;
    message_filters::Subscriber<jsk_recognition_msgs::ModelCoefficientsArray> sub_coefficients_;
    ros::Publisher debug_magnified_polygons_;
    ros::Publisher pub_grid_map_;
    boost::shared_ptr <dynamic_reconfigure::Server<Config> > srv_;
    
    
    ////////////////////////////////////////////////////////
    // Parametersp
    ////////////////////////////////////////////////////////
    
    double magnify_distance_;
    double distance_threshold_;
    double resolution_;
  private:
  };
}

#endif
