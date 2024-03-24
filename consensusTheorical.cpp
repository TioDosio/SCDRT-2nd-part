#include <iostream>
#include <cmath>

struct Node
{
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
};

void initializeNode(Node &node, double K[], int index, double cost, double lux, double o)
{
    node.index = index;
    for (int i = 0; i < 3; i++)
    {
        node.d[i] = 0;
        node.d_av[i] = 0;
        node.lambda[i] = 0;
        node.k[i] = K[i];
        node.c[i] = (i == index) ? cost : 0;
    }
    node.n = node.k[0] * node.k[0] + node.k[1] * node.k[1] + node.k[2] * node.k[2];
    node.m = node.n - node.k[index] * node.k[index];
    node.o = o;
    node.L = lux;
}

double evaluateCost(Node node, double d[], double rho)
{
    double cost = 0;
    double norm_squared = 0;

    for (int i = 0; i < 3; i++)
    {
        cost += node.c[i] * d[i];
        double diff = d[i] - node.d_av[i];
        cost += node.lambda[i] * (diff);
        norm_squared += diff * diff;
    }

    cost += rho / 2 * norm_squared;
    return cost;
}

bool checkFeasibility(Node node, double d[])
{
    double tol = 0.001;
    if (d[node.index] < 0 - tol)
    {
        return false;
    }
    if (d[node.index] > 100 + tol)
    {
        return false;
    }
    double dot_product = 0;
    for (int i = 0; i < 3; i++)
    {
        dot_product += d[i] * node.k[i];
    }
    if (dot_product < node.L - node.o - tol)
    {
        return false;
    }
    return true;
}

void updateBest(double d_best[], double d[], double &cost_best, double cost)
{
    for (int i = 0; i < 3; i++)
    {
        d_best[i] = d[i];
    }
    cost_best = cost;
}

void copyArray(double dest[], double src[])
{
    for (int i = 0; i < 3; i++)
    {
        dest[i] = src[i];
    }
}

void consensusIterate(Node &node, double rho, double d[], double &cost)
{
    int SIZE = 3;
    double y[SIZE];
    for (int i = 0; i < SIZE; i++)
    {
        y[i] = rho * node.d_av[i] - node.lambda[i] - node.c[i];
    }

    double d_best[SIZE];
    double cost_best = 1000000;

    double d_temp[SIZE];

    // Unconstrained minimum
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i];
    }
    if (checkFeasibility(node, d_temp))
    {
        double cost_temp = evaluateCost(node, d_temp, rho);
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
        d_temp[i] = (1 / rho) * y[i] - node.k[i] / node.n * (node.o - node.L + (1 / rho) * (y[0] * node.k[0] + y[1] * node.k[1] + y[2] * node.k[2]));
    }
    if (checkFeasibility(node, d_temp))
    {
        double cost_temp = evaluateCost(node, d_temp, rho);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Compute minimum constrained to 0 boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i];
        if (i == node.index)
        {
            d_temp[i] = 0;
        }
    }
    if (checkFeasibility(node, d_temp))
    {
        double cost_temp = evaluateCost(node, d_temp, rho);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Compute minimum constrained to 100 boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i];
        if (i == node.index)
        {
            d_temp[i] = 100;
        }
    }
    if (checkFeasibility(node, d_temp))
    {
        double cost_temp = evaluateCost(node, d_temp, rho);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Compute minimum constrained to linear and 0 boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i] - (1 / node.m) * node.k[i] * (node.o - node.L) +
                    (1 / rho / node.m) * node.k[i] * (node.k[node.index] * y[node.index] - (y[0] * node.k[0] + y[1] * node.k[1] + y[2] * node.k[2]));
        if (i == node.index)
        {
            d_temp[i] = 0;
        }
    }
    if (checkFeasibility(node, d_temp))
    {
        double cost_temp = evaluateCost(node, d_temp, rho);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Compute minimum constrained to linear and 100 boundary
    for (int i = 0; i < SIZE; i++)
    {
        d_temp[i] = (1 / rho) * y[i] - (1 / node.m) * node.k[i] * (node.o - node.L + 100 * node.k[node.index]) +
                    (1 / rho / node.m) * node.k[i] * (node.k[node.index] * y[node.index] - (y[0] * node.k[0] + y[1] * node.k[1] + y[2] * node.k[2]));
        if (i == node.index)
        {
            d_temp[i] = 100;
        }
    }
    if (checkFeasibility(node, d_temp))
    {
        double cost_temp = evaluateCost(node, d_temp, rho);
        if (cost_temp < cost_best)
        {
            updateBest(d_best, d_temp, cost_best, cost_temp);
        }
    }

    // Copy the best d and cost back to the original variables
    copyArray(d, d_best);
    cost = cost_best;
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
    initializeNode(node1, K[0], 0, c1, L1, o1);

    // node2
    Node node2;
    initializeNode(node2, K[1], 1, c2, L2, o2);

    // node3
    Node node3;
    initializeNode(node3, K[2], 2, c3, L3, o3);

    // iterations
    for (int i = 1; i < maxiter; i++)
    {
        // COMPUTATION OF THE PRIMAL SOLUTIONS
        // node 1
        double d1[3];
        double cost1;
        consensusIterate(node1, rho, d1, cost1);

        for (int j = 0; j < 3; j++)
        {
            node1.d[j] = d1[j];
        }
        // node 2
        double d2[3];
        double cost2;
        consensusIterate(node2, rho, d2, cost2);
        for (int j = 0; j < 3; j++)
        {
            node2.d[j] = d2[j];
        }

        // node 3
        double d3[3];
        double cost3;
        consensusIterate(node3, rho, d3, cost3);
        for (int j = 0; j < 3; j++)
        {
            node3.d[j] = d3[j];
        }

        // NODES EXCHANGE THEIR SOLUTIONS
        // (COMMUNICATIONS HERE)

        // COMPUTATION OF THE AVERAGE
        for (int j = 0; j < 3; j++)
        {
            node1.d_av[j] = (node1.d[j] + node2.d[j] + node3.d[j]) / 3;
            node2.d_av[j] = (node1.d[j] + node2.d[j] + node3.d[j]) / 3;
            node3.d_av[j] = (node1.d[j] + node2.d[j] + node3.d[j]) / 3;
        }

        // COMPUTATION OF THE LAGRANGIAN UPDATES
        for (int j = 0; j < 3; j++)
        {
            node1.lambda[j] = node1.lambda[j] + rho * (node1.d[j] - node1.d_av[j]);
            node2.lambda[j] = node2.lambda[j] + rho * (node2.d[j] - node2.d_av[j]);
            node3.lambda[j] = node3.lambda[j] + rho * (node3.d[j] - node3.d_av[j]);
        }
    }

    std::cout << "Consensus Solutions" << std::endl;
    std::cout << "d = ";
    for (int i = 0; i < 3; i++)
    {
        std::cout << node1.d_av[i] << " ";
    }
    std::cout << std::endl;
    std::cout << "l = ";
    for (int i = 0; i < 3; i++)
    {
        double l = K[i][0] * node1.d_av[0] + K[i][1] * node1.d_av[1] + K[i][2] * node1.d_av[2] + o[i];
        std::cout << l << " ";
    }
    std::cout << std::endl;

    return 0;
}
