/**
 * @file test_read_sdf.cpp
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief Test reading a sdf map from csv file.
 * @version 0.1
 * @date 2022-07-27
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#include "helpers/MatrixIO.h"
#include <gtest/gtest.h>

using namespace vimp;
using namespace Eigen;

TEST(SDFReader, read_csv){
    MatrixIO loader;
    MatrixXd sdf = loader.load_csv("data/map_ground_truth.csv");

    MatrixXd map_ground_truth = (MatrixXd(7, 7) <<
                0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0,
                0, 0, 1, 1, 1, 0, 0,
                0, 0, 1, 1, 1, 0, 0,
                0, 0, 1, 1, 1, 0, 0,
                0, 0, 0, 0, 0, 0, 0,
                0, 0, 0, 0, 0, 0, 0).finished();


    ASSERT_EQ((sdf - map_ground_truth).norm(), 0);
    
}


TEST(SDFReader, read_write){
    MatrixIO m_io;
    
    int dim = 300;
    MatrixXd m = MatrixXd::Random(dim, dim);
    
    m_io.saveData("tests/example.csv", m);

    MatrixXd m_read = m_io.load_csv("tests/example.csv");

    ASSERT_LE((m - m_read).norm(), 1e-10);
}

