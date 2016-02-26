/*****************************************************
             PROJECT  : lhcb-daqpipe
             DATE     : 07/2015
             AUTHOR   : Sébastien Valat - CERN
*****************************************************/

/********************  HEADERS  *********************/
//usage of PMI (hydra)
#include "hydra-simple/pmi.h"
//local
#include "../common/log.hpp"
//std
#include <cassert>
#include <cstdio>
#include <cstdlib>
//to implement
#include "LauncherHydra.hpp"

/*******************  NAMESPACE  ********************/
namespace DAQ
{

/********************  MACROS  **********************/
/**
 * Helper function to check MPI status. It will finish with abort() in case of failure.
 * @param funcName Define the name of the function which has been called.
 * @param status Define the output status to check.
**/
#define PMI_CHECK_STATUS(funcName,status) \
	do {\
		if (status != PMI_SUCCESS) {\
			fprintf(stderr,"Failed onto %s with code %d : %s",funcName,status,cstPMIErrors[status]);\
			abort();\
		}\
	} while(0)

/********************  GLOBALS  *********************/
/**
 * Array to convert MPI error ids to strings in PMI_CHECK_STATUS().
**/
const char * cstPMIErrors[] = {
	"SUCCESS",
	"FAIL",
	"ERR_INIT",
	"ERR_NOMEM",
	"ERR_INVALID",
	"ERR_INVALID_KEY",
	"ERR_INVALID_KEY",
	"ERR_INVALID_VAL",
	"ERR_INVALID_VAL_LENGTH",
	"ERR_INVALID_LENGTH",
	"ERR_INVALID_NUM_ARGS",
	"ERR_INVALID_ARGS",
	"ERR_INVALID_NUM_PARSED",
	"ERR_INVALID_KEYVALP",
	"ERR_INVALID_SIZE"
};

/*******************  FUNCTION  *********************/
/**
 * Setup the PMI by calling its init function and fetching the KVS name to register keys.
**/
void LauncherHydra::initialize ( int argc, char** argv )
{
	//init
	int spawned;
	int status = PMI_Init(&spawned);
	PMI_CHECK_STATUS("PMI_Init",status);
	
	//get KVS key size and check
	int size;
	status = PMI_KVS_Get_name_length_max(&size);
	PMI_CHECK_STATUS("PMI_KVS_Get_name_length_max",status);
	if (size >= 1024)
	{
		fprintf(stderr,"Too small buffer to get KVS name\n");
		abort();
	}
	
	//load kvs name
	status = PMI_KVS_Get_my_name( kvsName, sizeof(kvsName) );
	PMI_CHECK_STATUS("PMI_KVS_Get_my_name",status);
	
	//debugging
	LOG(DEBUG) << "hydra-launcher [" << getRank() << "] KVS name : " << kvsName;
}

/*******************  FUNCTION  *********************/
/**
 * Finish PMI on exit.
**/
void LauncherHydra::finalize ( void )
{
	PMI_Finalize();
}

/*******************  FUNCTION  *********************/
/**
 * Wait all nodes to be here.
**/
void LauncherHydra::barrier ( void )
{
	int status = PMI_Barrier();
	PMI_CHECK_STATUS("PMI_Barrier",status);
}

/*******************  FUNCTION  *********************/
/**
 * Return the number of processes in the job.
**/
int LauncherHydra::getWorldSize ( void )
{
	//get universe size
	int size;
	int status = PMI_Get_size(&size);
	PMI_CHECK_STATUS("PMI_Get_size",status);
	return size;
}

/*******************  FUNCTION  *********************/
/**
 * Return the rank of the current process in the job.
**/
int LauncherHydra::getRank ( void )
{
	//get rank
	int rank;
	int status = PMI_Get_rank(&rank);
	PMI_CHECK_STATUS("PMI_Get_rank",status);
	return rank;
}

/*******************  FUNCTION  *********************/
/**
 * Commit the new registered keys to made them visible to all.
**/
void LauncherHydra::commit ( void )
{
	int status = PMI_KVS_Commit(kvsName);
	PMI_CHECK_STATUS("PMI_KVS_Commit",status);
}

/*******************  FUNCTION  *********************/
/**
 * Register a new key in the KVS database. You need to use commit() before it
 * become visible to all tasks.
 * @param key Key to setup.
 * @param rank Associate value to given rank in addition to key.
 * @param value Value to attached to the key/rank.
**/
void LauncherHydra::set ( const std::string& key, const std::string& value )
{
	char buffer[512];
	sprintf(buffer,"%s-%d",key.c_str(),getRank());
	int status = PMI_KVS_Put(kvsName,buffer,value.c_str());
	PMI_CHECK_STATUS("PMI_KVS_Put",status);
}

/*******************  FUNCTION  *********************/
/**
 * Read the value attached to the given key.
 * @param key Key to read.
 * @param rank Read value from the given rank.
**/
std::string LauncherHydra::get ( const std::string& key, int rank )
{
	char buffer[512];
	char outputBuffer[1024];
	sprintf(buffer,"%s-%d",key.c_str(),rank);
	int status = PMI_KVS_Get(kvsName,buffer,outputBuffer,sizeof(outputBuffer));
	PMI_CHECK_STATUS("PMI_KVS_Get",status);
	return outputBuffer;
}

}
