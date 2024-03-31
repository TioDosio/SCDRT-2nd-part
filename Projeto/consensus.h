#ifndef CONSENSUS
#define CONSENSUS

#include <Arduino.h>
#include <cmath>

class Node
{
private:
    int index;
    double d[3];
    double d_av[3];
    double lambda[3];
    double k[3];
    double n;
    double m;
    double c[3];
    double o;
    double rho;
    double cost;
    double lastD[3] = {-1, -1, -1};
    double otherD[2][3];
    int occupancy = 0;
    double lowerBoundOccupied = 20;
    double lowerBoundUnoccupied = 5;
    double L = lowerBoundUnoccupied;
    bool consensusRunning = false;
    bool consensusReady = false;
    int consensusIteration = 0;
    int maxiter = 100;
    double evaluateCost(double d[]);
    bool checkFeasibility(double d[]);
    void updateBest(double d_best[], double d[], double &cost_best, double cost);

public:
    void initializeNode(double *K, int index, double o);
    void consensusIterate();
    bool checkConvergence();
    double getKIndex(int index);
    double getO();
    double *getDav();
    double getDavIndex(int index);
    void setDavIndex(int index, double value);
    double getDIndex(int index);
    void setD(double d[]);
    double *getD();
    double getLambdaIndex(int index);
    void setLambdaIndex(int index, double value);
    double getCost();
    void setCost(double value);
    double getRho();
    double *getLastD();
    void copyArray(double dest[], double src[]);
    void setLowerBoundOccupied(double value);
    double getLowerBoundOccupied();
    void setLowerBoundUnoccupied(double value);
    double getLowerBoundUnoccupied();
    void setOccupancy(int value);
    int getOccupancy();
    double getCurrentLowerBound();
    void setConsensusRunning(bool value);
    bool getConsensusRunning();
    void setConsensusIterations(int value);
    int getConsensusIterations();
    void setConsensusMaxIterations(int value);
    int getConsensusMaxIterations();
    void resetOtherD();
    bool checkOtherDIsFull();
    void setOtherD(int index, double d[]);
    double *getOtherD(int index);
    void setConsensusReady(bool value);
    bool getConsensusReady();
};

#endif