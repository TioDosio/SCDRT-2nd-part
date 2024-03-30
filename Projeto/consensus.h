#ifndef CONSENSUS
#define CONSENSUS

#include <Arduino.h>

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
    double L;
    double rho;
    double cost;
    double lastD[3];
    double otherD[2][3];
    int occupancy;
    double lowerBoundOccupied = 10;
    double lowerBoundUnoccupied = 3;
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
    double *getDav();
    double getDavIndex(int index);
    void setDavIndex(int index, double value);
    double getDIndex(int index);
    void setD(double d[]);
    double *getD();
    double getLambdaIndex(int index);
    void setLambdaIndex(int index, double value);
    double getCost();
    double getRho();
    double *getLastD();
    void copyArray(double dest[], double src[]);
    void setLowerBoundOccupied(double value);
    double getLowerBoundOccupied();
    void setLowerBoundUnoccupied(double value);
    double getLowerBoundUnoccupied();
    void setOccupancy(int value);
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