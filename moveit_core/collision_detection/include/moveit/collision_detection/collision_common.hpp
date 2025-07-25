/*********************************************************************
 * Software License Agreement (BSD License)
 *
 *  Copyright (c) 2011, Willow Garage, Inc.
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
 *     disclaimer in the documentation and/or other materials provided
 *     with the distribution.
 *   * Neither the name of Willow Garage nor the names of its
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

/* Author: Ioan Sucan */

#pragma once

#include <array>
#include <functional>
#include <vector>
#include <string>
#include <map>
#include <set>
#include <Eigen/Core>
#include <moveit/robot_model/robot_model.hpp>

namespace collision_detection
{
MOVEIT_CLASS_FORWARD(AllowedCollisionMatrix);  // Defines AllowedCollisionMatrixPtr, ConstPtr, WeakPtr... etc

/** \brief The types of bodies that are considered for collision */
namespace BodyTypes
{
/** \brief The types of bodies that are considered for collision */
enum Type
{
  /** \brief A link on the robot */
  ROBOT_LINK,

  /** \brief A body attached to a robot link */
  ROBOT_ATTACHED,

  /** \brief A body in the environment */
  WORLD_OBJECT
};
}  // namespace BodyTypes

/** \brief The types of bodies that are considered for collision */
using BodyType = BodyTypes::Type;

/** \brief Definition of a contact point */
struct Contact
{
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  /** \brief contact position */
  Eigen::Vector3d pos;

  /** \brief normal unit vector at contact */
  Eigen::Vector3d normal;

  /** \brief depth (penetration between bodies) */
  double depth = 0.0;

  /** \brief The id of the first body involved in the contact */
  std::string body_name_1;

  /** \brief The type of the first body involved in the contact */
  BodyType body_type_1;

  /** \brief The id of the second body involved in the contact */
  std::string body_name_2;

  /** \brief The type of the second body involved in the contact */
  BodyType body_type_2;

  /** \brief The distance percentage between casted poses until collision.
   *
   *  If the value is 0, then the collision occurred in the start pose. If the value is 1, then the collision occurred
   * in the end pose. */
  double percent_interpolation = 0.0;

  /** \brief The two nearest points connecting the two bodies */
  Eigen::Vector3d nearest_points[2];
};

/** \brief When collision costs are computed, this structure contains information about the partial cost incurred in a
 * particular volume */
struct CostSource
{
  /// The minimum bound of the AABB defining the volume responsible for this partial cost
  std::array<double, 3> aabb_min;

  /// The maximum bound of the AABB defining the volume responsible for this partial cost
  std::array<double, 3> aabb_max;

  /// The partial cost (the probability of existence for the object there is a collision with)
  double cost = 0.0;

  /// Get the volume of the AABB around the cost source
  double getVolume() const
  {
    return (aabb_max[0] - aabb_min[0]) * (aabb_max[1] - aabb_min[1]) * (aabb_max[2] - aabb_min[2]);
  }

  /// Order cost sources so that the most costly source is at the top
  bool operator<(const CostSource& other) const
  {
    double c1 = cost * getVolume();
    double c2 = other.cost * other.getVolume();
    if (c1 > c2)
      return true;
    if (c1 < c2)
      return false;
    if (cost < other.cost)
      return false;
    if (cost > other.cost)
      return true;
    return aabb_min < other.aabb_min;
  }
};

struct CollisionResult;

/** \brief Representation of a collision checking request */
struct CollisionRequest
{
  /** \brief The group name to check collisions for (optional; if empty, assume the complete robot). Descendent links
   * are included. */
  std::string group_name = "";

  /** \brief If true, use padded collision environment */
  bool pad_environment_collisions = true;

  /** \brief If true, do self collision check with padded robot links */
  bool pad_self_collisions = false;

  /** \brief If true, compute proximity distance */
  bool distance = false;

  /** \brief If true, return detailed distance information. Distance must be set to true as well */
  bool detailed_distance = false;

  /** \brief If true, a collision cost is computed */
  bool cost = false;

  /** \brief If true, compute contacts. Otherwise only a binary collision yes/no is reported. */
  bool contacts = false;

  /** \brief Overall maximum number of contacts to compute */
  std::size_t max_contacts = 1;

  /** \brief Maximum number of contacts to compute per pair of bodies (multiple bodies may be in contact at different
   * configurations) */
  std::size_t max_contacts_per_pair = 1;

  /** \brief When costs are computed, this value defines how many of the top cost sources should be returned */
  std::size_t max_cost_sources = 1;

  /** \brief Function call that decides whether collision detection should stop. */
  std::function<bool(const CollisionResult&)> is_done = nullptr;

  /** \brief Flag indicating whether information about detected collisions should be reported */
  bool verbose = false;
};

namespace DistanceRequestTypes
{
enum DistanceRequestType
{
  GLOBAL,   ///< Find the global minimum
  SINGLE,   ///< Find the global minimum for each pair
  LIMITED,  ///< Find a limited(max_contacts_per_body) set of contacts for a given pair
  ALL       ///< Find all the contacts for a given pair
};
}
using DistanceRequestType = DistanceRequestTypes::DistanceRequestType;

/** \brief Representation of a distance-reporting request */
struct DistanceRequest
{
  /*** \brief Compute \e active_components_only_ based on \e req_. This
      includes links that are in the kinematic model but outside this group, if those links are descendants of
      joints in this group that have their values updated. */
  void enableGroup(const moveit::core::RobotModelConstPtr& robot_model)
  {
    if (robot_model->hasJointModelGroup(group_name))
    {
      active_components_only = &robot_model->getJointModelGroup(group_name)->getUpdatedLinkModelsSet();
    }
    else
    {
      active_components_only = nullptr;
    }
  }

  /// Indicate if nearest point information should be calculated
  bool enable_nearest_points = false;

  /// Indicate if a signed distance should be calculated in a collision.
  bool enable_signed_distance = false;

  /// Indicate the type of distance request. If using type=ALL, it is
  /// recommended to set max_contacts_per_body to the expected number
  /// of contacts per pair because it is used to reserve space.
  DistanceRequestType type = DistanceRequestType::GLOBAL;

  /// Maximum number of contacts to store for bodies (multiple bodies may be within distance threshold)
  std::size_t max_contacts_per_body = 1;

  /// The group name
  std::string group_name = "";

  /// The set of active components to check
  const std::set<const moveit::core::LinkModel*>* active_components_only = nullptr;

  /// The allowed collision matrix used to filter checks
  const AllowedCollisionMatrix* acm = nullptr;

  /// Only calculate distances for objects within this threshold to each other.
  /// If set, this can significantly reduce the number of queries.
  double distance_threshold = std::numeric_limits<double>::max();

  /// Log debug information
  bool verbose = false;

  /// Indicate if gradient should be calculated between each object.
  /// This is the normalized vector connecting the closest points on the two objects.
  bool compute_gradient = false;
};

/** \brief Generic representation of the distance information for a pair of objects */
// TODO(#267): Enable check - for some reason clang-tidy wants to rename this struct to "i0"
struct DistanceResultsData  // NOLINT(readability-identifier-naming) - suppress spurious clang-tidy warning
{
  DistanceResultsData()
  {
    clear();
  }

  /// The distance between two objects. If two objects are in collision, distance <= 0.
  double distance;

  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  /// The nearest points
  Eigen::Vector3d nearest_points[2];

  /// The object link names
  std::string link_names[2];

  /// The object body type
  BodyType body_types[2];

  /** Normalized vector connecting closest points (from link_names[0] to link_names[1])

      Usually, when checking convex to convex, the normal is connecting closest points.
      However, FCL in case of non-convex to non-convex or convex to non-convex returns
      the contact normal for one of the two triangles that are in contact. */
  Eigen::Vector3d normal;

  /// Clear structure data
  void clear()
  {
    distance = std::numeric_limits<double>::max();
    nearest_points[0].setZero();
    nearest_points[1].setZero();
    body_types[0] = BodyType::WORLD_OBJECT;
    body_types[1] = BodyType::WORLD_OBJECT;
    link_names[0] = "";
    link_names[1] = "";
    normal.setZero();
  }

  /// Compare if the distance is less than another
  bool operator<(const DistanceResultsData& other)
  {
    return (distance < other.distance);
  }

  /// Compare if the distance is greater than another
  bool operator>(const DistanceResultsData& other)
  {
    return (distance > other.distance);
  }
};

/** \brief Mapping between the names of the collision objects and the DistanceResultData. */
using DistanceMap = std::map<std::pair<std::string, std::string>, std::vector<DistanceResultsData>>;

/** \brief Result of a distance request. */
struct DistanceResult
{
  /// Indicates if two objects were in collision
  bool collision = false;

  /// ResultsData for the two objects with the minimum distance
  DistanceResultsData minimum_distance;

  /// A map of distance data for each link in the req.active_components_only
  DistanceMap distances;

  /// Clear structure data
  void clear()
  {
    collision = false;
    minimum_distance.clear();
    distances.clear();
  }
};

/** \brief Representation of a collision checking result */
struct CollisionResult
{
  EIGEN_MAKE_ALIGNED_OPERATOR_NEW

  /** \brief Clear a previously stored result */
  void clear()
  {
    collision = false;
    distance = std::numeric_limits<double>::max();
    distance_result.clear();
    contact_count = 0;
    contacts.clear();
    cost_sources.clear();
  }

  /** \brief Throttled warning printing the first collision pair, if any. All collisions are logged at DEBUG level */
  void print() const;

  /** \brief True if collision was found, false otherwise */
  bool collision = false;

  /** \brief Closest distance between two bodies */
  double distance = std::numeric_limits<double>::max();

  /** \brief Distance data for each link */
  DistanceResult distance_result;

  /** \brief Number of contacts returned */
  std::size_t contact_count = 0;

  /** \brief A map returning the pairs of body ids in contact, plus their contact details */
  using ContactMap = std::map<std::pair<std::string, std::string>, std::vector<Contact>>;
  ContactMap contacts;

  /** \brief These are the individual cost sources when costs are computed */
  std::set<CostSource> cost_sources;
};
}  // namespace collision_detection
