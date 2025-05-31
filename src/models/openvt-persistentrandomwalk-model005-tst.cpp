/*

Copyright 1996-2006 Roeland Merks

This file is part of Tissue Simulation Toolkit.

Tissue Simulation Toolkit is free software; you can redistribute
it and/or modify it under the terms of the GNU General Public
License as published by the Free Software Foundation; either
version 2 of the License, or (at your option) any later version.

Tissue Simulation Toolkit is distributed in the hope that it will
be useful, but WITHOUT ANY WARRANTY; without even the implied
warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.
See the GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with Tissue Simulation Toolkit; if not, write to the Free
Software Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
02110-1301 USA

*/
#include <stdio.h>
#ifndef __APPLE__
#include <malloc.h>
#endif
#include "cell.hpp"
#include "dish.hpp"
#include "graph.hpp"
#include "info.hpp"
#include "inputoutput.hpp"
#include "parameter.hpp"
#include "plotter.hpp"
#include "profiler.hpp"
#include "random.hpp"
#include "sqr.hpp"
#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <math.h>
#include <nlohmann/json.hpp>
#include <iomanip>  // <-- For std::fixed and std::setprecision
#include <filesystem>



using json = nlohmann::json;
int id=0;

struct Model {
   double cpm_temperature;
   double cpm_area_c;
   double cpm_area_v;
   double cpm_perim_c;
   double cpm_perim_v;
   int len_1;
   int len_2;
   int cpm_nbs_n;
   int cpm_surface_nbs_n;
   int max_time;
   std::string method;
   std::string model;
   json model_args;
};

struct Simulation {
   int num_sims;
   std::string output_name;
   int output_per;
   std::vector<std::vector<int>> init_voxels;
};


Model model;
Simulation sim;

void parse_json(const std::string& filename, Model &model, Simulation &sim) {
    std::ifstream file(filename);
    if (!file.is_open()) {
        std::cerr << "Error opening file: " << filename << std::endl;
        return;
    }

    json j;
    file >> j;

    // Parse model
    model = {
        j["model"]["cpm_temperature"].get<double>(),
        j["model"]["cpm_area_c"].get<double>(),
        j["model"]["cpm_area_v"].get<double>(),
        j["model"]["cpm_perim_c"].get<double>(),
        j["model"]["cpm_perim_v"].get<double>(),
        j["model"]["len_1"].get<int>(),
        j["model"]["len_2"].get<int>(),
        j["model"]["cpm_nbs_n"].get<int>(),
        j["model"]["cpm_surface_nbs_n"].get<int>(),
        j["model"]["max_time"].get<int>(),
        j["model"]["method"].get<std::string>(),
        j["model"]["model"].get<std::string>(),
        j["model"]["model_args"]  // 👈 store whole object
    };

    // Parse sim
    sim = {
        j["sim"]["num_sims"].get<int>(),
        j["sim"]["output_name"].get<std::string>(),
        j["sim"]["output_per"].get<int>(),
        j["sim"]["init_voxels"].get<std::vector<std::vector<int>>>()
    };
}

/*void export_json_data(std::vector<double> &time,
    std::vector<int> &id,
    std::vector<double> &com_1,
    std::vector<double> &com_2,
    std::vector<double> &area,
    std::vector<double> &surface,
                      ) {

    // Construct the data items
    json data = json::array({
        {
            {"key", "time"},
            {"description", "Simulation times"},
            {"type", "array"},
            {"items", {{"type", "number"}}},
            {"values", time}
        },
        {
            {"key", "id"},
            {"description", "Integer identifier of the cell"},
            {"type", "array"},
            {"items", {{"type", "integer"}}},
            {"values", id}
        },
        {
            {"key", "com_1"},
            {"description", "Centroid positions along first spatial dimension"},
            {"type", "array"},
            {"items", {{"type", "number"}}},
            {"values", com_1}
        },
        {
            {"key", "com_2"},
            {"description", "Centroid positions along second spatial dimension"},
            {"type", "array"},
            {"items", {{"type", "number"}}},
            {"values", com_2}
        },
        {
            {"key", "area"},
            {"description", "Area occupied by the cell"},
            {"type", "array"},
            {"items", {{"type", "number"}}},
            {"values", area}
        },
        {
            {"key", "surface"},
            {"description", "Surface length of the cell"},
            {"type", "array"},
            {"items", {{"type", "number"}}},
            {"values", surface}
        }
    });

    // Wrap in the top-level object
    json output = {
        {"data", data}
    };

    // Write to file
    std::ofstream file("output.json");
    file << output.dump(4);  // Pretty print with 4-space indentation
    file.close();

    std::cout << "JSON data exported to output.json\n";

    return 0;
}*/

using namespace std;

INIT {
  try {
    if (par.initial_configuration_file ==
        "None") { 
        // If no configuration file is provided
      // Define initial distribution of cells
        cout << par.sizex << ", " << par.sizey << "\n";
        //CPM->GrowInCells(par.n_init_cells, par.size_init_cells, par.subfield);
       for (auto it=sim.init_voxels.begin(); it!=sim.init_voxels.end(); it++) {
            int ix=(*it)[0], iy=(*it)[1];
            CPM->setSigma(ix,iy,1);
        }
       // CPM->setSigma(25,25,1);
        CPM->ConstructInitCells(*this);
        CPM->MeasureCellPerimeters();
        Cell::SetJ(0,1,0);
        double target_angle = model.model_args["initial_alpha"].get<double>();
        
        
        CPM->getCell(1).SetMovementAngle(MovementTracker::Coordinate(cos(target_angle),sin(target_angle)));
        
        
        
        
    } else { // If a configuration file is provided
      io->ReadConfiguration();
    }

    CPM->InitialiseEdgeList();

  } catch (const char *error) {
    cerr << "Caught exception\n";
    std::cerr << error << "\n";
    exit(1);
  }
}

TIMESTEP {
  try {

    static int i = 0;
    static Dish *dish;
    if (i == 0) {
      dish = new Dish();
    }

    static Info *info = new Info(*dish, *this);
    static Plotter *plotter = 0;
    if (par.graphics && plotter == 0) {
       plotter = new Plotter(dish, this);
    }
      static ofstream *cellpos_stream = nullptr;
      static IO *io = 0;
      if (io == 0 ) {
          io = new IO(*dish);
      }
      

    if (par.graphics && !(i % par.storage_stride)) {
      PROFILE(all_plots, plotter->Plot();)
      info->Menu();
      dish->CPM->FindBoundingBox(); // old: Setboundingbox
    }

    if (i == 0 && par.pause_on_start) {
      info->set_Paused();
      i++;
    }

    if (!info->IsPaused()) {
      PROFILE(amoebamove, dish->CPM->AmoebaeMove(dish->PDEfield);)
        
        dish->CPM->CalcPeriodicSafeCentroids();
        dish->getCell(1).TrackPosition();
        MovementTracker::Coordinate displacement = dish->getCell(1).MovementVector();
        double displacement_magnitude = dish->getCell(1).MovementMagnitude();
        if (displacement_magnitude > 0.) {
            MovementTracker::Coordinate new_alpha(displacement.first/displacement_magnitude,displacement.second/displacement_magnitude);
            dish->getCell(1).SetMovementAngle(new_alpha);
        }
        
    }
    // cout << "Compactness = " << dish-> CPM -> Compactness() << endl;

    /*if (i == par.mcs) {
      dish->ExportMultiCellDS(par.mcds_output);
    }*/

    if (par.store && !(i % par.storage_stride)) {
     
        // Build the folder name
        
        std::ostringstream folder_name;
        folder_name << "images_" << id;
        
        std::filesystem::path dir = par.datadir + "/" + sim.output_name + "/" + folder_name.str();
        std::filesystem::create_directories(dir);
        
        // build the filename
        std::ostringstream fname_part;
        fname_part << "snapshot_" << std::setw(5) << std::setfill('0') << i << ".png";
        // Create full path
        std::filesystem::path full_path = dir / fname_part.str();
        std::string fname = full_path.string();

        // Copy into mutable null-terminated char array
        std::vector<char> fstr(fname.begin(), fname.end());
        fstr.push_back('\0');  // Ensure null termination

        std::cerr << "filename: " << fstr.data() << std::endl;
        Write(fstr.data());    // Now safe to call Write(char*)
    }

      {
          
          
          if (!(i%sim.output_per)) { // open file if necessary and write data
              if (cellpos_stream == nullptr) {
                  std::cout << "Opening cell position" << std::endl;
                  
                  // Construct the output file path
                  std::filesystem::path dir = par.datadir + "/" + sim.output_name;
                  std::filesystem::create_directories(dir); // Recursively create the directory if it doesn't exist
                  
                  std::filesystem::path cpfname = std::filesystem::path(dir) / "tst.csv";
                  
                  bool file_exists = std::filesystem::exists(cpfname);
                  
                  // Open file in append mode
                  cellpos_stream = new std::ofstream(cpfname, std::ios::app);
                  if (!(*cellpos_stream)) {
                      std::cerr << "Failed to open file: " << cpfname << std::endl;
                      exit(1);
                  }
                  
                  // Ensure the stream uses a locale that supports UTF-8 (optional, can omit if unnecessary)
                  cellpos_stream->imbue(std::locale(std::locale(), new std::numpunct<char>));
                  
                  // Set fixed-point notation and a specific precision
                  *cellpos_stream << std::scientific << std::setprecision(6);  // Adjust precision as needed
                  
                  // Write the CSV header
                  if (!file_exists) {
                      *cellpos_stream << "time,id,com_1,com_2,area,surface" << std::endl;
                  }
              }
              
              // Retrieve data from cell with ID 1
              double cx = dish->getCell(1).getCenterX();
              double cy = dish->getCell(1).getCenterY();
              double carea = dish->getCell(1).Area();
              double cperimeter = dish->getCell(1).Perimeter();
              
              // Write the data row
              *cellpos_stream << i << ","    // Simulation time or timestep
              << id << ","    // Cell ID 
              << cx << ","
              << cy << ","
              << carea << ","
              << cperimeter
              << std::endl;
              cellpos_stream->flush(); // after each time step, if needed
          }
      }
    if (!info->IsPaused()) {
      i++;
    }
  } catch (const char *error) {
    cerr << "Caught exception\n";
    std::cerr << error << "\n";
    exit(1);
  }
  PROFILE_PRINT
}

void Plotter::Plot() {
  graphics->BeginScene();
  graphics->ClearImage();

  plotCPMCellTypes();
  plotCPMLines();

  graphics->EndScene();
}

void PDE::DerivativesPDE(CellularPotts *cpm, PDEFIELD_TYPE *derivs, int x,
                         int y) {}

int PDE::MapColour(double val) {
  return (((int)((val / ((val) + 1.)) * 100)) % 100) + 155;
}

int main(int argc, char *argv[]) {
  extern Parameter par;
    if (argc > 2) {
           try {
               id = std::stoi(argv[2]);  // Convert the string to int using std::stoi
           }
           catch (const std::invalid_argument& e) {
               std::cerr << "Invalid argument: " << argv[2] << " is not a valid integer." << std::endl;
               return 1;  // Exit with an error code if conversion fails
           }
           catch (const std::out_of_range& e) {
               std::cerr << "Out of range: The value is too large for an int." << std::endl;
               return 1;
           }
       } else {
           std::cerr << "Insufficient arguments. Please provide an argument for id." << std::endl;
           return 1;  // Exit with an error code if not enough arguments
       }

       // Continue with the rest of your program
       std::cout << "id = " << id << std::endl;
        try {
            par.Read(argv[1]);
            
                // read json file and override some configs
                
                parse_json("input-model005.json",model, sim);
                
                // Copy parsed data to Parameter object
                par.T = model.cpm_temperature;
                std::cout << "T: " << par.T << std::endl;
                par.target_area = model.cpm_area_c;
                std::cout << "target_area" <<  par.target_area << std::endl;
                par.lambda = model.cpm_area_v;
                std::cout << "lambda" <<  par.lambda << std::endl;
                par.target_perimeter = model.cpm_perim_c;
                std::cout << "target_perimeter" <<  par.target_perimeter << std::endl;
                par.lambda_perimeter = model.cpm_perim_v;
                std::cout << "lambda_perimeter" <<  par.lambda_perimeter << " = " << std::endl;
                par.neighbours = model.cpm_nbs_n;
                std::cout << "Neighbours: " << par.neighbours << std::endl;
                par.mcs = model.max_time;
                std::cout << "mcs = " <<  par.mcs << std::endl;
                par.sizex = model.len_1;
                par.sizey = model.len_2;
            par.lambda_move = model.model_args["lambda_dir"].get<double>();
            par.cell_move_dt = model.model_args["dt"].get<int>();
            
            
           // cerr << "par.lambda_move: " << par.lambda_move << endl;
            //std::string mode = model.model_args["cpm_force_mode"];
            
                
                Seed(par.rseed);
                
                start_graphics(argc, argv);
    
        } catch (const char *error) {
            std::cerr << error << std::endl;
            return 1;
        }
    
  return 0;
}
