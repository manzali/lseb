/*****************************************************
             PROJECT  : lhcb-daqpipe
             DATE     : 07/2015
             AUTHOR   : Sébastien Valat - CERN
*****************************************************/

#ifndef HYDRA_LAUNCHER_HPP
#define HYDRA_LAUNCHER_HPP

/********************  HEADERS  *********************/
#include <string>

/*******************  NAMESPACE  ********************/
namespace lseb
{

/*********************  CLASS  **********************/
/**
 * Provide a wrapper to hide the low level functions from PMI (hydra).
**/
class HydraLauncher
{
  public:
    void initialize ( int argc, char** argv );
    void finalize(void);
    int getRank(void);
    int getWorldSize(void);
    void barrier(void);
    void set(const std::string & key,const std::string & value);
    std::string get(const std::string & key,int rank);
    void commit(void);
  private:
    /** Store the KVS DB name to access keys/values. **/
    char kvsName[1024];
};

}

#endif //HYDRA_LAUNCHER_HPP
