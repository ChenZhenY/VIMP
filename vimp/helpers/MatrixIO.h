/**
 * @file MatrixIO.h
 * @author Hongzhe Yu (hyu419@gatech.edu)
 * @brief Helpers related to the data io stream
 * @version 0.1
 * @date 2022-07-16
 * 
 * @copyright Copyright (c) 2022
 * 
 */

#pragma once

#include "helpers/repeated_includes.h"
#include <vector>

namespace vimp{
    class MatrixIO{
    public:
        
        MatrixIO(){}

        template <typename T>
        void saveData(const std::string& fileName, const T& matrix, bool verbose=true) const{
            if (verbose){
                std::cout << "Saving data to: " << fileName << std::endl;
            }
            std::ofstream file(fileName);
            if (file.is_open()){
                file << matrix.format(CSVFormat);
                file.close();
            }
        }

        /**
         * @brief read a sdf map from csv file, which can be the output of some gpmp2 functions.
         * modified from an online code piece.
         * @param filename 
         * @return Matrix 
         */
        /// https://stackoverflow.com/questions/34247057/how-to-read-csv-file-and-assign-to-eigen-matrix

        Eigen::MatrixXd load_csv (const std::string & path) {
            std::ifstream indata;
            indata.open(path);
            if (indata.peek() == std::ifstream::traits_type::eof()){
                throw std::runtime_error(std::string("File dose not exist ...: ") + path);
            }
            
            std::string line;
            std::vector<double> values;
            uint rows = 0;
            while (std::getline(indata, line)) {
                std::stringstream lineStream(line);
                std::string cell;
                while (std::getline(lineStream, cell, ',')) {
                    values.push_back(std::stod(cell));
                }
                rows++;
            }
            return Eigen::Map<Eigen::Matrix<double, Eigen::Dynamic, Eigen::Dynamic, Eigen::RowMajor>>(values.data(), rows, values.size()/rows);
        }

    };

}

