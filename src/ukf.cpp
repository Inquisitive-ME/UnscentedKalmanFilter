#include "ukf.h"
#include "Eigen/Dense"
#include <iostream>

using namespace std;
using Eigen::MatrixXd;
using Eigen::VectorXd;
using std::vector;

/**
 * Initializes Unscented Kalman filter
 * This is scaffolding, do not modify
 */
UKF::UKF() {
  is_initialized_ = false;

  previous_timestamp_ = 0;

  // if this is false, laser measurements will be ignored (except during init)
  use_laser_ = true;

  // if this is false, radar measurements will be ignored (except during init)
  use_radar_ = true;

  // initial state vector
  x_ = VectorXd(5);

  // initial covariance matrix
  P_ = MatrixXd(5, 5);
  // Tune
  P_ << 0.1, 0, 0, 0, 0,
        0, 0.1, 0, 0, 0,
        0, 0, 3, 0, 0,
        0, 0, 0, 0.5, 0,
        0, 0, 0, 0, 0.5;

  // Process noise standard deviation longitudinal acceleration in m/s^2
  std_a_ = 3.5/2; // Tune

  // Process noise standard deviation yaw acceleration in rad/s^2
  std_yawdd_ = M_PI/4; // Tune
  
  //DO NOT MODIFY measurement noise values below these are provided by the sensor manufacturer.
  // Laser measurement noise standard deviation position1 in m
  std_laspx_ = 0.15;

  // Laser measurement noise standard deviation position2 in m
  std_laspy_ = 0.15;

  // Radar measurement noise standard deviation radius in m
  std_radr_ = 0.3;

  // Radar measurement noise standard deviation angle in rad
  std_radphi_ = 0.03;

  // Radar measurement noise standard deviation radius change in m/s
  std_radrd_ = 0.3;
  //DO NOT MODIFY measurement noise values above these are provided by the sensor manufacturer.

  // State dimension
  n_x_ = 5;

  // Augmented state dimension
  n_aug_ = 7;

  // Sigma point spreading parameter
  lambda_ = 3 - n_x_;

  // predicted sigma points matrix
  Xsig_pred_= MatrixXd(n_x_, 2 * n_aug_ + 1);
  Xsig_pred_.fill(0.0);

  // Weights of sigma points
  weights_ = VectorXd(2*n_aug_+1);

}

UKF::~UKF() {}

/**
 * @param {MeasurementPackage} meas_package The latest measurement data of
 * either radar or laser.
 */
void UKF::ProcessMeasurement(MeasurementPackage meas_package) {
  /**
  TODO:

  Complete this function! Make sure you switch between lidar and radar
  measurements.
  */

    /*****************************************************************************
   *  Initialization
   ****************************************************************************/
  if (!is_initialized_)
  {

    if (meas_package.sensor_type_ == MeasurementPackage::RADAR && use_radar_)
    {
      /**
      Convert radar from polar to cartesian coordinates and initialize state.
      */
      x_ << meas_package.raw_measurements_[0] * cos(meas_package.raw_measurements_[1]),
            meas_package.raw_measurements_[0] * sin(meas_package.raw_measurements_[1]),
            1.0,
            0.0,
            0.0;
      is_initialized_ = true;
    }
    else if (meas_package.sensor_type_ == MeasurementPackage::LASER && use_laser_)
    {
      x_ << meas_package.raw_measurements_[0],
            meas_package.raw_measurements_[1],
            1.0,
            0.0,
            0.0;
      is_initialized_ = true;
    }
    previous_timestamp_ = meas_package.timestamp_;
  }

  if(is_initialized_)
  {
    /*****************************************************************************
   *  Prediction
   ****************************************************************************/
    float dt = (meas_package.timestamp_ - previous_timestamp_) / 1000000.0;
    Prediction(dt);

    /*****************************************************************************
     *  Update
     ****************************************************************************/
     Update(meas_package);
  }

  previous_timestamp_ = meas_package.timestamp_;

}

/**
 * Predicts sigma points, the state, and the state covariance matrix.
 * @param {double} delta_t the change in time (in seconds) between the last
 * measurement and this one.
 */
void UKF::Prediction(double delta_t) {
  /**
  TODO:

  Complete this function! Estimate the object's location. Modify the state
  vector, x_. Predict sigma points, the state, and the state covariance matrix.
  */

  // Augment Sigma Points
  CalculateSigmaPoints(delta_t);

  PredictXandP();
}

void UKF::CalculateSigmaPoints(double delta_t)
{
  //create augmented mean vector
  VectorXd x_aug = VectorXd(n_aug_);

  //create augmented state covariance
  MatrixXd P_aug = MatrixXd(n_aug_, n_aug_);

  //create sigma point matrix
  MatrixXd Xsig_aug = MatrixXd(n_aug_, 2 * n_aug_ + 1);

  //create augmented mean state
  x_aug.head(n_x_) = x_;
  x_aug(n_x_) = 0;
  x_aug(n_x_ + 1) = 0;

  //create augmented covariance matrix
  P_aug.fill(0.0);
  P_aug.topLeftCorner(n_x_,n_x_) = P_;
  P_aug(n_x_,n_x_) = std_a_*std_a_;
  P_aug(n_x_+1,n_x_+1) = std_yawdd_*std_yawdd_;

  //create square root matrix
  MatrixXd L = P_aug.llt().matrixL();

  //create augmented sigma points
  Xsig_aug.col(0)  = x_aug;
  for (int i = 0; i< n_aug_; i++)
  {
    Xsig_aug.col(i+1)       = x_aug + sqrt(lambda_+n_aug_) * L.col(i);
    Xsig_aug.col(i+1+n_aug_) = x_aug - sqrt(lambda_+n_aug_) * L.col(i);
  }

  // Predict Sigma Points
  //predict sigma points
  VectorXd x_k_1 = VectorXd(n_x_);
  VectorXd noise = VectorXd(n_x_);

  for(int i = 0; i < (n_aug_ *2 +1); i++)
  {
    if(Xsig_aug(4, i) == 0)
    {
      x_k_1 << Xsig_aug(2,i)*cos(Xsig_aug(3,i))*delta_t,
          Xsig_aug(2,i)*sin(Xsig_aug(3,i))*delta_t,
          0,
          0,
          0;
    }
    else
    {
      x_k_1 << Xsig_aug(2,i)/Xsig_aug(4,i) * (sin(Xsig_aug(3,i) + Xsig_aug(4,i)*delta_t) - sin(Xsig_aug(3,i))),
          Xsig_aug(2,i)/Xsig_aug(4,i) * (-cos(Xsig_aug(3,i) + Xsig_aug(4,i)*delta_t) + cos(Xsig_aug(3,i))),
          0,
          Xsig_aug(4,i) * delta_t,
          0;
    }
    noise << 0.5*delta_t*delta_t*cos(Xsig_aug(3,i))*Xsig_aug(5,i),
        0.5*delta_t*delta_t*sin(Xsig_aug(3,i))*Xsig_aug(5,i),
        delta_t * Xsig_aug(5,i),
        0.5*delta_t*delta_t*Xsig_aug(6,i),
        delta_t*Xsig_aug(6,i);

    Xsig_pred_.col(i) = Xsig_aug.col(i).head(n_x_) + x_k_1 + noise;
  }
}

void UKF::PredictXandP()
{
    /*******************************************************************************
  * Predict Mean and Covariance
  ******************************************************************************/
  // set weights
  double weight_0 = lambda_/(lambda_+n_aug_);
  weights_(0) = weight_0;
  for (int i=1; i<2*n_aug_+1; i++) {  //2n+1 weights
    double weight = 0.5/(n_aug_+lambda_);
    weights_(i) = weight;
  }

  //predicted state mean
  x_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points
    x_ = x_+ weights_(i) * Xsig_pred_.col(i);
  }

  //predicted state covariance matrix
  P_.fill(0.0);
  for (int i = 0; i < 2 * n_aug_ + 1; i++) {  //iterate over sigma points

    // state difference
    VectorXd x_diff = Xsig_pred_.col(i) - x_;
    //angle normalization
    while (x_diff(3)> M_PI)
    {
      x_diff(3)-=2.*M_PI;
    }
    while (x_diff(3)<-M_PI)
    {
      x_diff(3)+=2.*M_PI;
    }

    P_ = P_ + weights_(i) * x_diff * x_diff.transpose() ;
  }
}

void UKF::Update(MeasurementPackage meas_package)
{
  bool LidarMeasurement = (use_laser_ && meas_package.sensor_type_ == MeasurementPackage::LASER);
  bool RadarMeasurement = (use_radar_ && meas_package.sensor_type_ == MeasurementPackage::RADAR);

  if(LidarMeasurement | RadarMeasurement)
  {
    VectorXd z = meas_package.raw_measurements_;

    int n_z = 0;
    if(LidarMeasurement)
    {
      n_z = 2;
    }
    else if(RadarMeasurement)
    {
      n_z = 3;
    }

    //create matrix for sigma points in measurement space
    MatrixXd Zsig = MatrixXd(n_z, 2 * n_aug_ + 1);

    //transform sigma points into measurement space
    for (int i = 0; i < 2 * n_aug_ + 1; i++)
    {  //2n+1 simga points

      if(LidarMeasurement)
      {
        // extract values for better readibility
        double p_x = Xsig_pred_(0, i);
        double p_y = Xsig_pred_(1, i);

        // measurement model
        Zsig(0, i) = p_x;      //px
        Zsig(1, i) = p_y;      //py
      }
      else if(RadarMeasurement)
      {
        // extract values for better readibility
        double p_x = Xsig_pred_(0,i);
        double p_y = Xsig_pred_(1,i);
        double v  = Xsig_pred_(2,i);
        double yaw = Xsig_pred_(3,i);

        double v1 = cos(yaw)*v;
        double v2 = sin(yaw)*v;

        // measurement model
        Zsig(0,i) = sqrt(p_x*p_x + p_y*p_y);                        //r
        Zsig(1,i) = atan2(p_y,p_x);                                 //phi
        Zsig(2,i) = (p_x*v1 + p_y*v2 ) / sqrt(p_x*p_x + p_y*p_y);   //r_dot
      }
    }

    //mean predicted measurement
    VectorXd z_pred = VectorXd(n_z);
    z_pred.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++)
    {
      z_pred = z_pred + weights_(i) * Zsig.col(i);
    }

    //innovation covariance matrix S
    MatrixXd S = MatrixXd(n_z, n_z);
    S.fill(0.0);
    for (int i = 0; i < 2 * n_aug_ + 1; i++)
    {  //2n+1 simga points
      //residual
      VectorXd z_diff = Zsig.col(i) - z_pred;

      if(RadarMeasurement)
      {
        while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
        while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
      }

      S = S + weights_(i) * z_diff * z_diff.transpose();
    }

    if(LidarMeasurement)
    {
      //add measurement noise covariance matrix
      MatrixXd R = MatrixXd(n_z, n_z);
      R << std_laspx_ * std_laspx_, 0,
          0, std_laspy_ * std_laspy_;
      S = S + R;
    }
    else if (RadarMeasurement)
    {
      //add measurement noise covariance matrix
      MatrixXd R = MatrixXd(n_z,n_z);
      R <<    std_radr_*std_radr_, 0, 0,
          0, std_radphi_*std_radphi_, 0,
          0, 0,std_radrd_*std_radrd_;
      S = S + R;
    }

    MatrixXd Tc = MatrixXd(n_x_, n_z);
    Tc.fill(0.0);
    //calculate cross correlation matrix
    for (int i = 0; i < 2 * n_aug_ + 1; i++)
    {  //2n+1 simga points

      //residual
      VectorXd z_diff = Zsig.col(i) - z_pred;
      if(RadarMeasurement)
      {
        while (z_diff(1)> M_PI) z_diff(1)-=2.*M_PI;
        while (z_diff(1)<-M_PI) z_diff(1)+=2.*M_PI;
      }

      // state difference
      VectorXd x_diff = Xsig_pred_.col(i) - x_;
      //angle normalization
      while (x_diff(3) > M_PI) x_diff(3) -= 2. * M_PI;
      while (x_diff(3) < -M_PI) x_diff(3) += 2. * M_PI;

      Tc = Tc + weights_(i) * x_diff * z_diff.transpose();
    }

    //Kalman gain K;
    MatrixXd K = Tc * S.inverse();

    //residual
    VectorXd z_diff = z - z_pred;

    //angle normalization
    while (z_diff(1) > M_PI) z_diff(1) -= 2. * M_PI;
    while (z_diff(1) < -M_PI) z_diff(1) += 2. * M_PI;

    //update state mean and covariance matrix
    x_ = x_ + K * z_diff;
    P_ = P_ - K * S * K.transpose();

    if(LidarMeasurement)
    {
      NIS_Lidar_.push_back(z_diff.transpose() * S.inverse() * z_diff);
    }
    else if (RadarMeasurement)
    {
      NIS_Radar_.push_back(z_diff.transpose() * S.inverse() * z_diff);
    }
  }
}
