#include <iostream>
#include <cmath>
#include "consensus.h"

void runConsensus(Node &node1, Node &node2, Node &node3, int maxiter)
{
    // iterations
    for (int i = 1; i < maxiter; i++)
    {
        // COMPUTATION OF THE PRIMAL SOLUTIONS
        // node 1
        node1.consensusIterate();

        // node 2
        node2.consensusIterate();

        // node 3
        node3.consensusIterate();

        // NODES EXCHANGE THEIR SOLUTIONS
        // (COMMUNICATIONS HERE)

        // COMPUTATION OF THE AVERAGE
        double temp;
        for (int j = 0; j < 3; j++)
        {
            temp = (node1.getDIndex(j) + node2.getDIndex(j) + node3.getDIndex(j)) / 3;
            node1.setDavIndex(j, temp);
            node2.setDavIndex(j, temp);
            node3.setDavIndex(j, temp);
        }

        // COMPUTATION OF THE LAGRANGIAN UPDATES
        for (int j = 0; j < 3; j++)
        {
            node1.setLambdaIndex(j, node1.getLambdaIndex(j) + node1.getRho() * (node1.getDIndex(j) - node1.getDavIndex(j)));
            node2.setLambdaIndex(j, node2.getLambdaIndex(j) + node2.getRho() * (node2.getDIndex(j) - node2.getDavIndex(j)));
            node3.setLambdaIndex(j, node3.getLambdaIndex(j) + node3.getRho() * (node3.getDIndex(j) - node3.getDavIndex(j)));
        }
    }
}

int main()
{
    // EXPERIMENTAL CASE
    double L1 = 120, o1 = 0, L2 = 100, o2 = 20, L3 = 120, o3 = 0;

    // COST FUNCTION PARAMETERS
    // symmetric costs
    double c1 = 1, c2 = 1, c3 = 1;

    // SOLVER PARAMETERS
    double rho = 0.1;
    int maxiter = 50;

    // SYSTEM CALIBRATION PARAMETERS
    double k11 = 2, k12 = 1, k13 = 1, k21 = 1, k22 = 2, k23 = 1, k31 = 1, k32 = 1, k33 = 2;

    // VARIABLES FOR CENTRALIZED SOLUTION
    double K[3][3] = {{k11, k12, k13}, {k21, k22, k23}, {k31, k32, k33}};
    double c[3] = {c1, c2, c3};
    double L[3] = {L1, L2, L3};
    double o[3] = {o1, o2, o3};

    // DISTRIBUTED NODE INITIALIZATION

    // node1
    Node node1;
    node1.initializeNode(K[0], 0, c1, L1, o1, rho);

    // node2
    Node node2;
    node2.initializeNode(K[1], 1, c2, L2, o2, rho);

    // node3
    Node node3;
    node3.initializeNode(K[2], 2, c3, L3, o3, rho);

    // RUN CONSENSUS ALGORITHM
    runConsensus(node1, node2, node3, maxiter);

    std::cout << "Consensus Solutions" << std::endl;
    std::cout << "d = ";
    for (int i = 0; i < 3; i++)
    {
        std::cout << node1.getDavIndex(i) << " ";
    }
    std::cout << std::endl;
    std::cout << "l = ";
    for (int i = 0; i < 3; i++)
    {
        double l = K[i][0] * node1.getDavIndex(0) + K[i][1] * node1.getDavIndex(1) + K[i][2] * node1.getDavIndex(2) + o[i];
        std::cout << l << " ";
    }
    std::cout << std::endl;

    return 0;
}
