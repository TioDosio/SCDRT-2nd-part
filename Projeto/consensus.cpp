#include <iostream>
#include <cmath>
#include "consensus.h"

void Node::initializeNode(double *K, int index, double o)
{
    this->index = index;
    rho = 0.7;
    for (int i = 0; i < 3; i++)
    {
        d[i] = 0;
        lastD[i] = 0;
        d_av[i] = 0;
        lambda[i] = 0;
        k[i] = K[i];
        c[i] = (i == index) ? 1 : 0; // MUDAR O 1 PARA O CUSTO
    }
    n = k[0] * k[0] + k[1] * k[1] + k[2] * k[2];
    m = n - k[index] * k[index];
    this->o = o;
    L = 30.0;
}

double Node::evaluateCost(double d[])
{
    double costTemp = 0;
    double norm_squared = 0;

    for (int i = 0; i < 3; i++)
    {
        costTemp += c[i] * d[i];
        double diff = d[i] - d_av[i];
        costTemp += lambda[i] * (diff);
        norm_squared += diff * diff;
    }

    costTemp += rho / 2 * norm_squared;
    return costTemp;
}

bool Node::checkFeasibility(double d[])
{
    double tol = 0.001;
    if (d[index] < 0 - tol)
    {
        return false;
    }
    if (d[index] > 100 + tol)
    {
        return false;
    }
    double dot_product = 0;
    for (int i = 0; i < 3; i++)
    {
        dot_product += d[i] * k[i];
    }
    if (dot_product < L - o - tol)
    {
        return false;
    }
    return true;
}

void Node::updateBest(double d_best[], double d[], double &cost_best, double cost)
{
    for (int i = 0; i < 3; i++)
    {
        d_best[i] = d[i];
    }
    cost_best = cost;
}

void Node::copyArray(double dest[], double src[])
{
    for (int i = 0; i < 3; i++)
    {
        dest[i] = src[i];
    }
}

void Node::consensusIterate()
{
    int SIZE = 3;
    double y[SIZE];
    for (int i = 0; i < SIZE; i++)
    {
        y[i] = rho * d_av[i] - lambda[i] - c[i];
    }

    double d_best[SIZE];
    double cost_best = 1000000;

    double d_temp[SIZE];

    // Unconstrained minimum
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i];
    }
    if (checkFeasibility(d_temp))
    {
        double cost_temp = evaluateCost(d_temp);
        if (cost_temp < cost_best)
        {
            copyArray(d, d_temp); // if unconstrained is the best, no other will be
            cost = cost_temp;
            return;
        }
    }

    // Compute minimum constrained to linear boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i] - k[i] / n * (o - L + (1 / rho) * (y[0] * k[0] + y[1] * k[1] + y[2] * k[2]));
    }
    if (checkFeasibility(d_temp))
    {
        double cost_temp = evaluateCost(d_temp);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Compute minimum constrained to 0 boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i];
        if (i == index)
        {
            d_temp[i] = 0;
        }
    }
    if (checkFeasibility(d_temp))
    {
        double cost_temp = evaluateCost(d_temp);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Compute minimum constrained to 100 boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i];
        if (i == index)
        {
            d_temp[i] = 100;
        }
    }
    if (checkFeasibility(d_temp))
    {
        double cost_temp = evaluateCost(d_temp);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Compute minimum constrained to linear and 0 boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i] - (1 / m) * k[i] * (o - L) +
                    (1 / rho / m) * k[i] * (k[index] * y[index] - (y[0] * k[0] + y[1] * k[1] + y[2] * k[2]));
        if (i == index)
        {
            d_temp[i] = 0;
        }
    }
    if (checkFeasibility(d_temp))
    {
        double cost_temp = evaluateCost(d_temp);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Compute minimum constrained to linear and 100 boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i] - (1 / m) * k[i] * (o - L + 100 * k[index]) +
                    (1 / rho / m) * k[i] * (k[index] * y[index] - (y[0] * k[0] + y[1] * k[1] + y[2] * k[2]));
        if (i == index)
        {
            d_temp[i] = 100;
        }
    }
    if (checkFeasibility(d_temp))
    {
        double cost_temp = evaluateCost(d_temp);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Copy the best d and cost back to the original variables
    copyArray(d, d_best);
    cost = cost_best;
}

bool Node::checkConvergence()
{
    double tol = 0.01;
    double norm_squared = 0;
    if (k[0] * getDavIndex(0) + k[1] * getDavIndex(1) + k[2] * getDavIndex(2) + o < L)
    {
        return false;
    }
    for (int i = 0; i < 3; i++)
    {
        double diff = d[i] - lastD[i];
        norm_squared += diff * diff;
    }
    if (norm_squared < tol)
    {
        return true;
    }
    return false;
}

double *Node::getDav()
{
    return d_av;
}

double Node::getDavIndex(int index)
{
    return d_av[index];
}

void Node::setDavIndex(int index, double value)
{
    d_av[index] = value;
}

double Node::getDIndex(int index)
{
    return d[index];
}

void Node::setD(double d[])
{
    for (int i = 0; i < 3; i++)
    {
        this->d[i] = d[i];
    }
}

double *Node::getD()
{
    return d;
}

double Node::getLambdaIndex(int index)
{
    return lambda[index];
}

void Node::setLambdaIndex(int index, double value)
{
    lambda[index] = value;
}

double Node::getCost()
{
    return cost;
}

double Node::getRho()
{
    return rho;
}

double *Node::getLastD()
{
    return lastD;
}

void Node::setLowerBoundOccupied(double value)
{
    lowerBoundOccupied = value;
    L = lowerBoundOccupied;
}

double Node::getLowerBoundOccupied()
{
    return lowerBoundOccupied;
}

void Node::setLowerBoundUnoccupied(double value)
{
    lowerBoundUnoccupied = value;
    L = lowerBoundUnoccupied;
}

double Node::getLowerBoundUnoccupied()
{
    return lowerBoundUnoccupied;
}

void Node::setOccupancy(int value)
{
    if (value == 1)
    {
        occupancy = 1;
        L = getLowerBoundOccupied();
    }
    else if (value == 0)
    {
        occupancy = 0;
        L = getLowerBoundUnoccupied();
    }
}

double Node::getCurrentLowerBound()
{
    return L;
}

void Node::setConsensusRunning(bool value)
{
    consensusRunning = value;
}

bool Node::getConsensusRunning()
{
    return consensusRunning;
}

void Node::setConsensusIterations(int value)
{
    consensusIteration = value;
}

int Node::getConsensusIterations()
{
    return consensusIteration;
}

void Node::setConsensusMaxIterations(int value)
{
    maxiter = value;
}

int Node::getConsensusMaxIterations()
{
    return maxiter;
}

void Node::resetOtherD()
{
    for (int i = 0; i < 2; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            otherD[i][j] = -1;
        }
    }
}

bool Node::checkOtherDIsFull()
{
    if (otherD[0][0] != -1 && otherD[1][0] != -1)
    {
        return true;
    }
    return false;
}

void Node::setOtherD(int index, double *value)
{
    for (int i = 0; i < 3; i++)
    {
        otherD[index][i] = value[i];
    }
}

double *Node::getOtherD(int index)
{
    return otherD[index];
}

void Node::setConsensusReady(bool value)
{
    consensusReady = value;
}

bool Node::getConsensusReady()
{
    return consensusReady;
}
