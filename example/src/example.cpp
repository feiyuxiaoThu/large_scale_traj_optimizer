#include "traj_min_jerk.hpp"
#include "traj_min_snap.hpp"

#include <chrono>

#include <cmath>
#include <iostream>
#include <string>
#include <vector>
#include <random>


using namespace std;
using namespace Eigen;

class RandomRouteGenerator
{
public:
    RandomRouteGenerator(Array3d l, Array3d u)
        : lBound(l), uBound(u), uniformReal(0.0, 1.0) {}

    inline MatrixXd generate(int N)
    {
        MatrixXd route(3, N + 1);
        Array3d temp;
        route.col(0).setZero();
        for (int i = 0; i < N; i++)
        {
            temp << uniformReal(gen), uniformReal(gen), uniformReal(gen);
            temp = (uBound - lBound) * temp + lBound;
            route.col(i + 1) << temp;
        }
        return route;
    }

private:
    Array3d lBound;
    Array3d uBound;
    std::mt19937_64 gen;
    std::uniform_real_distribution<double> uniformReal;
};

VectorXd allocateTime(const MatrixXd &wayPs,
                      double vel,
                      double acc)
{
    int N = (int)(wayPs.cols()) - 1;
    VectorXd durations(N);
    if (N > 0)
    {

        Eigen::Vector3d p0, p1;
        double dtxyz, D, acct, accd, dcct, dccd, t1, t2, t3;
        for (int k = 0; k < N; k++)
        {
            p0 = wayPs.col(k);
            p1 = wayPs.col(k + 1);
            D = (p1 - p0).norm();

            acct = vel / acc;
            accd = (acc * acct * acct / 2);
            dcct = vel / acc;
            dccd = acc * dcct * dcct / 2;

            if (D < accd + dccd)
            {
                t1 = sqrt(acc * D) / acc;
                t2 = (acc * t1) / acc;
                dtxyz = t1 + t2;
            }
            else
            {
                t1 = acct;
                t2 = (D - accd - dccd) / vel;
                t3 = dcct;
                dtxyz = t1 + t2 + t3;
            }

            durations(k) = dtxyz;
        }
    }

    return durations;
}

int main(int argc, char **argv)
{


    RandomRouteGenerator routeGen(Array3d(0, 0, 0), Array3d(20, -4.5, 0));

    min_jerk::JerkOpt jerkOpt;
    min_jerk::Trajectory minJerkTraj;

    min_snap::SnapOpt snapOpt;
    min_snap::Trajectory minSnapTraj;

    MatrixXd route;
    VectorXd ts;
    Matrix3d iS, fS;
    Eigen::Matrix<double, 3, 4> iSS, fSS;
    iS.setZero();
    fS.setZero();
    Vector3d zeroVec(0.0, 0.0, 0.0);
   
    int groupSize = 5;

    std::chrono::high_resolution_clock::time_point tc0, tc1, tc2;
    double d0, d1;

    for (int i = 2; i <= 2; i++)
    {
        d0 = d1 = 0.0;
        for (int j = 0; j < groupSize; j++)
        {
            route = routeGen.generate(i);
            std::cout << "route debug" << route << std::endl;
            std::cout <<"route piece" << route.cols() -1 << std::endl;
            iS.col(0) << route.leftCols<1>();
            fS.col(0) << route.rightCols<1>();
            ts = allocateTime(route, 3.0, 3.0); // here
            std::cout << "important point allocateTime" <<  ts << std::endl;

            iSS << iS, Eigen::MatrixXd::Zero(3, 1);
            fSS << fS, Eigen::MatrixXd::Zero(3, 1);

            tc0 = std::chrono::high_resolution_clock::now();
            jerkOpt.reset(iS, fS, route.cols() - 1);
            jerkOpt.generate(route.block(0, 1, 3, i - 1), ts);
            jerkOpt.getTraj(minJerkTraj);
            tc1 = std::chrono::high_resolution_clock::now();

            

            d0 += std::chrono::duration_cast<std::chrono::duration<double>>(tc1 - tc0).count();

            tc1 = std::chrono::high_resolution_clock::now();
            snapOpt.reset(iSS, fSS, route.cols() - 1);
            snapOpt.generate(route.block(0, 1, 3, i - 1), ts);
            snapOpt.getTraj(minSnapTraj);
            tc2 = std::chrono::high_resolution_clock::now();

            d1 += std::chrono::duration_cast<std::chrono::duration<double>>(tc2 - tc1).count();
        }

        /* For debug
            Use this for trajectory generation
        */

        //Piece1 = 
        min_jerk::Piece Piece1 = minJerkTraj[0];
        min_jerk::Piece Piece2 = minJerkTraj[0];

        std::cout << "piece1 duration t" << Piece1.getDuration() << std::endl;

        std::cout << "trajectory piece CoefficientMat" << "\t" << "Piece1" << "pos" << "\n"<< Piece1.getCoeffMat() << "\n"<<"\t" << "vel" << "\n" << Piece1.getVelCoeffMat()
          <<"\n"<<"\t" << "acc" << "\n" << Piece1.getAccCoeffMat()<< std::endl;
        //std::cout << "trajectory piece CoefficientMat" << "\t" << "Piece2" << "pos"<< Piece2.getCoeffMat() << "\t" << "vel" << Piece2.getVelCoeffMat()
        //  <<"\t" << "acc" << Piece2.getAccCoeffMat()<< std::endl;

        

        std::cout << "trajectory size" <<minJerkTraj.getPieceNum() << std::endl;


        std::cout << " whole Duration =" << minJerkTraj.getTotalDuration() <<"\n" << "Durations ="<< "\n" << minJerkTraj.getDurations() << "\n" << "piece num = " << minJerkTraj.getPieceNum()<<"\n" << "piece bounds" << "\n" << minJerkTraj.getPositions() << std::endl;

        //std::cout << "pos coff" << "\n" << minJerkTraj.getPos() << "\n" << "vel coff" << minJerkTraj.getV

        std::cout << "trajectory maxvel"<< "\t"  << minJerkTraj.getMaxVelRate() << "\t" << "max acc"<< "\t"  << minJerkTraj.getMaxAccRate()<< std::endl;

        std::cout << "trajectory position" << "\t" << minJerkTraj.getPositions() << "\t" << std::endl;

        double t_out1 = 5.89218;
        std::cout << "trajectory position at t=" << t_out1 << "\n" << minJerkTraj.getPos(t_out1)<<std::endl;
        double t_out2 = 8.72185;
        std::cout << "trajectory position at t=" << t_out2 << "\n" << minJerkTraj.getPos(t_out2)<<std::endl;

        std::cout << "Piece Number: " << i
                  << " MinJerk Comp. Time: " << d0 / groupSize << " s"
                  << " MinSnap Comp. Time: " << d1 / groupSize << " s" << std::endl;

      
    }

    return 0;
}
