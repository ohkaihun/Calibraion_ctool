// Copyright (c) 2023, ETH Zurich and UNC Chapel Hill.
// All rights reserved.
//
// Redistribution and use in source and binary forms, with or without
// modification, are permitted provided that the following conditions are met:
//
//     * Redistributions of source code must retain the above copyright
//       notice, this list of conditions and the following disclaimer.
//
//     * Redistributions in binary form must reproduce the above copyright
//       notice, this list of conditions and the following disclaimer in the
//       documentation and/or other materials provided with the distribution.
//
//     * Neither the name of ETH Zurich and UNC Chapel Hill nor the names of
//       its contributors may be used to endorse or promote products derived
//       from this software without specific prior written permission.
//
// THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
// AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
// IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
// ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDERS OR CONTRIBUTORS BE
// LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
// CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
// SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
// INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
// CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
// ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
// POSSIBILITY OF SUCH DAMAGE.

#pragma once

#include "colmap/geometry/rigid3.h"
#include "colmap/sensor/models.h"
#include "colmap/sensor/models_refrac.h"
#include "colmap/sensor/ray3d.h"
#include "colmap/util/eigen_alignment.h"
#include "colmap/util/types.h"

#include <vector>

#include <Eigen/Geometry>
#include <glog/logging.h>

namespace colmap {

// Camera class that holds the intrinsic parameters. Cameras may be shared
// between multiple images, e.g., if the same "physical" camera took multiple
// pictures with the exact same lens and intrinsics (focal length, etc.).
// This class has a specific distortion model defined by a camera model class.
struct Camera {
  // The unique identifier of the camera.
  camera_t camera_id = kInvalidCameraId;

  // The identifier of the camera model.
  CameraModelId model_id = CameraModelId::kInvalid;

  // The identifier of the refractive camera model. If the camera refractive
  // model is not specified or the camera is not refractive, the identifier is
  // `kInvalidCameraRefracModelId`
  CameraRefracModelId refrac_model_id = CameraRefracModelId::kInvalid;

  // The dimensions of the image, 0 if not initialized.
  size_t width = 0;
  size_t height = 0;

  // The focal length, principal point, and extra parameters. If the camera
  // model is not specified, this vector is empty.
  std::vector<double> params;

  // Refractive camera model parameters. If the camera refractive model is not
  // specified, this vector is empty.
  std::vector<double> refrac_params;

  // Whether there is a safe prior for the focal length,
  // e.g. manually provided or extracted from EXIF
  bool has_prior_focal_length = false;

  // Initialize parameters for given camera model and focal length, and set
  // the principal point to be the image center.
  static Camera CreateFromModelId(camera_t camera_id,
                                  CameraModelId model_id,
                                  double focal_length,
                                  size_t width,
                                  size_t height);
  static Camera CreateFromModelName(camera_t camera_id,
                                    const std::string& model_name,
                                    double focal_length,
                                    size_t width,
                                    size_t height);

  inline const std::string& ModelName() const;
  inline const std::string& RefracModelName() const;

  // Access focal length parameters.
  double MeanFocalLength() const;
  inline double FocalLength() const;
  inline double FocalLengthX() const;
  inline double FocalLengthY() const;
  inline void SetFocalLength(double f);
  inline void SetFocalLengthX(double fx);
  inline void SetFocalLengthY(double fy);

  // Access principal point parameters. Only works if there are two
  // principal point parameters.
  inline double PrincipalPointX() const;
  inline double PrincipalPointY() const;
  inline void SetPrincipalPointX(double cx);
  inline void SetPrincipalPointY(double cy);

  // Get the indices of the parameter groups in the parameter vector.
  inline span<const size_t> FocalLengthIdxs() const;
  inline span<const size_t> PrincipalPointIdxs() const;
  inline span<const size_t> ExtraParamsIdxs() const;

  // Get intrinsic calibration matrix composed from focal length and principal
  // point parameters, excluding distortion parameters.
  Eigen::Matrix3d CalibrationMatrix() const;

  // Get human-readable information about the parameter vector ordering.
  inline const std::string& ParamsInfo() const;
  inline const std::string& RefracParamsInfo() const;

  // Concatenate parameters as comma-separated list.
  std::string ParamsToString() const;
  std::string RefracParamsToString() const;

  // Set camera parameters from comma-separated list.
  bool SetParamsFromString(const std::string& string);

  // Set refractive parameters from comma-separated list.
  bool SetRefracParamsFromString(const std::string& string);

  // Check whether parameters are valid, i.e. the parameter vector has
  // the correct dimensions that match the specified camera model.
  inline bool VerifyParams() const;

  // Check whether refractive parameters are valid, i.e. the parameter vector
  // has the correct dimensions that match the specified refractive camera
  // model.
  inline bool VerifyRefracParams() const;

  // Check whether camera is already undistorted
  bool IsUndistorted() const;

  // Check whether the camera is a refractive camera.
  bool IsCameraRefractive() const;

  // Check whether camera has bogus parameters.
  inline bool HasBogusParams(double min_focal_length_ratio,
                             double max_focal_length_ratio,
                             double max_extra_param) const;

  // Project point in image plane to world / infinity.
  inline Eigen::Vector2d CamFromImg(const Eigen::Vector2d& image_point) const;

  // Convert pixel threshold in image plane to camera frame.
  inline double CamFromImgThreshold(double threshold) const;

  // Project point from camera frame to image plane.
  inline Eigen::Vector2d ImgFromCam(const Eigen::Vector2d& cam_point) const;

  // Project point in image plane to world as 3D ray using refractive camera
  // model.
  //
  // (note that the total reflection case is not handled here, if there is a
  // total reflection event happening, the refracted ray becomes (-nan -nan
  // -nan))
  inline Ray3D CamFromImgRefrac(const Eigen::Vector2d& image_point) const;

  // Project point in image plane to world given a depth.
  inline Eigen::Vector3d CamFromImgRefracPoint(
      const Eigen::Vector2d& image_point, double depth) const;

  // Project point from camera frame to image plane using refractive camera
  // model.
  //
  // (note that the total reflection case is not handled here, if there is a
  // total reflection event happening, the projection becomes (-nan -nan))
  inline Eigen::Vector2d ImgFromCamRefrac(
      const Eigen::Vector3d& cam_point) const;

  // Rescale camera dimensions and accordingly the focal length and
  // and the principal point.
  void Rescale(double scale);
  void Rescale(size_t new_width, size_t new_height);

  // Get the indices of the parameter groups in the parameter vector.
  const std::vector<size_t>& OptimizableRefracParamsIdxs() const;

  // Return the refraction axis if the camera is refractive.
  Eigen::Vector3d RefractionAxis() const;

  Eigen::Vector3d VirtualCameraCenter(const Ray3D& ray_refrac) const;

  // If the camera is refractive, this function returns a simple pinhole virtual
  // camera which observes the `cam_point` from the `image_point` perspectively.
  Camera VirtualCamera(const Eigen::Vector2d& image_point,
                       const Eigen::Vector2d& cam_point) const;

  void ComputeVirtual(const Eigen::Vector2d& point2D,
                      Camera& virtual_camera,
                      Rigid3d& virtual_from_real) const;

  void ComputeVirtuals(const std::vector<Eigen::Vector2d>& points2D,
                       std::vector<Camera>& virtual_cameras,
                       std::vector<Rigid3d>& virtual_from_reals) const;
};

////////////////////////////////////////////////////////////////////////////////
// Implementation
////////////////////////////////////////////////////////////////////////////////

const std::string& Camera::ModelName() const {
  return CameraModelIdToName(model_id);
}

const std::string& Camera::RefracModelName() const {
  return CameraRefracModelIdToName(refrac_model_id);
}

double Camera::FocalLength() const {
  const span<const size_t> idxs = FocalLengthIdxs();
  DCHECK_EQ(idxs.size(), 1);
  return params[idxs[0]];
}

double Camera::FocalLengthX() const {
  const span<const size_t> idxs = FocalLengthIdxs();
  return params[idxs[0]];
}

double Camera::FocalLengthY() const {
  const span<const size_t> idxs = FocalLengthIdxs();
  return params[idxs[(idxs.size() == 1) ? 0 : 1]];
}

void Camera::SetFocalLength(const double f) {
  const span<const size_t> idxs = FocalLengthIdxs();
  for (const size_t idx : idxs) {
    params[idx] = f;
  }
}

void Camera::SetFocalLengthX(const double fx) {
  const span<const size_t> idxs = FocalLengthIdxs();
  DCHECK_EQ(idxs.size(), 2);
  params[idxs[0]] = fx;
}

void Camera::SetFocalLengthY(const double fy) {
  const span<const size_t> idxs = FocalLengthIdxs();
  DCHECK_EQ(idxs.size(), 2);
  params[idxs[1]] = fy;
}

double Camera::PrincipalPointX() const {
  const span<const size_t> idxs = PrincipalPointIdxs();
  DCHECK_EQ(idxs.size(), 2);
  return params[idxs[0]];
}

double Camera::PrincipalPointY() const {
  const span<const size_t> idxs = PrincipalPointIdxs();
  DCHECK_EQ(idxs.size(), 2);
  return params[idxs[1]];
}

void Camera::SetPrincipalPointX(const double cx) {
  const span<const size_t> idxs = PrincipalPointIdxs();
  DCHECK_EQ(idxs.size(), 2);
  params[idxs[0]] = cx;
}

void Camera::SetPrincipalPointY(const double cy) {
  const span<const size_t> idxs = PrincipalPointIdxs();
  DCHECK_EQ(idxs.size(), 2);
  params[idxs[1]] = cy;
}

const std::string& Camera::ParamsInfo() const {
  return CameraModelParamsInfo(model_id);
}

const std::string& Camera::RefracParamsInfo() const {
  return CameraRefracModelParamsInfo(refrac_model_id);
}

span<const size_t> Camera::FocalLengthIdxs() const {
  return CameraModelFocalLengthIdxs(model_id);
}

span<const size_t> Camera::PrincipalPointIdxs() const {
  return CameraModelPrincipalPointIdxs(model_id);
}

span<const size_t> Camera::ExtraParamsIdxs() const {
  return CameraModelExtraParamsIdxs(model_id);
}

bool Camera::VerifyParams() const {
  return CameraModelVerifyParams(model_id, params);
}

bool Camera::VerifyRefracParams() const {
  return CameraRefracModelVerifyParams(refrac_model_id, refrac_params);
}

bool Camera::HasBogusParams(const double min_focal_length_ratio,
                            const double max_focal_length_ratio,
                            const double max_extra_param) const {
  return CameraModelHasBogusParams(model_id,
                                   params,
                                   width,
                                   height,
                                   min_focal_length_ratio,
                                   max_focal_length_ratio,
                                   max_extra_param);
}

Eigen::Vector2d Camera::CamFromImg(const Eigen::Vector2d& image_point) const {
  return CameraModelCamFromImg(model_id, params, image_point).hnormalized();
}

double Camera::CamFromImgThreshold(const double threshold) const {
  return CameraModelCamFromImgThreshold(model_id, params, threshold);
}

Eigen::Vector2d Camera::ImgFromCam(const Eigen::Vector2d& cam_point) const {
  return CameraModelImgFromCam(model_id, params, cam_point.homogeneous());
}

Ray3D Camera::CamFromImgRefrac(const Eigen::Vector2d& image_point) const {
  return CameraRefracModelCamFromImg(
      model_id, refrac_model_id, params, refrac_params, image_point);
}

Eigen::Vector3d Camera::CamFromImgRefracPoint(
    const Eigen::Vector2d& image_point, const double depth) const {
  return CameraRefracModelCamFromImgPoint(
      model_id, refrac_model_id, params, refrac_params, image_point, depth);
}

Eigen::Vector2d Camera::ImgFromCamRefrac(
    const Eigen::Vector3d& cam_point) const {
  return CameraRefracModelImgFromCam(
      model_id, refrac_model_id, params, refrac_params, cam_point);
}

}  // namespace colmap
