/*
 COMPILAR: g++ RunT.cpp -o Run -lpthread
*/

#include <iostream>
#include <cstdlib>
#include <string>
#include <vector>
#include <map>
#include <boost/algorithm/string/erase.hpp>
#include <fstream>
#include <sstream>
#include <pthread.h>

using namespace std;

//GLOBAL VARIABLES

string MergePassPath, PADriverPassPath, AliasSetPassPath, NetLevelPassPath, NetDepGraphPassPath; //pass Path
string graphFlag, statsFlag, debugFlag; //Flags

vector< vector<string> > parametersList, C_SFList, C_RFList, D_SFList, D_RFList; //Parameters

//FUNCTIONS

void system_call(char *cmd) 
{
	cout << "cmd=" << cmd << endl;
	system(cmd);
}

bool openConfigFile(map<string,string> &passPathAndFlags)
{
	string line;

	ifstream file_config;
	
	bool fileExists;

	file_config.open("run-config.txt");
		if (!file_config.is_open()) {
			cout << "Could not open configuration file \n";
		} else {
			cout << "Opened configuration file \n";
			fileExists = true;

			while (getline(file_config, line)) {
				if (line.find("MergePassPath=") != std::string::npos) {
					boost::erase_all(line, "MergePassPath=");
					passPathAndFlags["MergePassPath"] = line;
				}
				if (line.find("PADriverPassPath=") != std::string::npos) {
					boost::erase_all(line, "PADriverPassPath=");
					passPathAndFlags["PADriverPassPath"] = line;
				}
				if (line.find("AliasSetPassPath=") != std::string::npos) {
					boost::erase_all(line, "AliasSetPassPath=");
					passPathAndFlags["AliasSetPassPath"] = line;
				}
                if (line.find("NetLevelPassPath=") != std::string::npos) {
                    boost::erase_all(line, "NetLevelPassPath=");
                    passPathAndFlags["NetLevelPassPath"] = line;
                }
				if (line.find("NetDepGraphPassPath=") != std::string::npos) {
					boost::erase_all(line, "NetDepGraphPassPath=");
					passPathAndFlags["NetDepGraphPassPath"] = line;
				}

				if (line.find("GRAPH=") != std::string::npos) {
					//boost::erase_all(line, "NetDepGraphPassPath=");
					passPathAndFlags["graphFlag"] = line;
				}
				if (line.find("-debug") != std::string::npos) {
					//boost::erase_all(line, "NetDepGraphPassPath=");
					passPathAndFlags["debugFlag"] = line;
				}
				if (line.find("-stats") != std::string::npos) {
					//boost::erase_all(line, "NetDepGraphPassPath=");
					passPathAndFlags["statsFlag"] = line;
				}
			}
		}

		if(fileExists)
			return true;
		else
			return false;
}

bool openDataFile(string dataFileName, vector< vector < string > > &parametersList, vector< vector < string > > &C_SFList,
		vector< vector < string > > &C_RFList, vector< vector < string > > &D_SFList, vector< vector < string > > &D_RFList)
{
	string line;

	ifstream file_data;

	vector<string> parameters, C_SF, C_RF, D_SF, D_RF;

	file_data.open(dataFileName.c_str());
	if (!file_data.is_open())
	{
		cout << "Could not open " << dataFileName << "\n";
		return false;
	}
	else
	{
		cout << "Opened " << dataFileName << "\n";

		while(getline(file_data, line))
		{
			parameters.clear();
			C_SF.clear();
			C_RF.clear();
			D_SF.clear();
			D_RF.clear();

//			if (!getline(file_data, line))
//				break;

			istringstream ss(line);

			while (getline(ss, line, ','))
			{
				if (line.find("[sfc]") != std::string::npos)
				{
					boost::erase_all(line, "[sfc]");
					C_SF.push_back(line);
				}
				else if (line.find("[rfc]") != std::string::npos)
				{
					boost::erase_all(line, "[rfc]");
					C_RF.push_back(line);
				}
				else if (line.find("[sfd]") != std::string::npos)
				{
					boost::erase_all(line, "[sfd]");
					D_SF.push_back(line);
				}
				else if (line.find("[rfd]") != std::string::npos)
				{
					boost::erase_all(line, "[rfd]");
					D_RF.push_back(line);
				}
				else if (parameters.size() < 3)
				{
					parameters.push_back(line);
				}
			}

			parametersList.push_back(parameters);
			C_SFList.push_back(C_SF);
			C_RFList.push_back(C_RF);
			D_SFList.push_back(D_SF);
			D_RFList.push_back(D_RF);

		}

	}
	return true;

}

void findClientAndDaemonPossiblesSendRecv(string NameClient, string NameDaemon)
{
	string optMergeFindDaemonPossiblesSendRecvCommand, optMergeFindClientPossiblesSendRecvCommand;

	cout << endl << "Looking for possibles send/recv functions for " << NameDaemon << " and "<< NameClient << endl;

	optMergeFindDaemonPossiblesSendRecvCommand = "opt -load " + MergePassPath
			+ " -Merge -FLG=0 <" + NameDaemon
			+ ".bc> " + NameDaemon +"_Daemonfuncnames.txt -o /tmp/temp 2>&1";
	system_call((char*) optMergeFindDaemonPossiblesSendRecvCommand.c_str());

	optMergeFindClientPossiblesSendRecvCommand = "opt -load " + MergePassPath
			+ " -Merge -FLG=0 <" + NameClient
			+ ".bc>" + NameClient +"_Clientfuncnames.txt -o /tmp/temp 2>&1";
	system_call((char*) optMergeFindClientPossiblesSendRecvCommand.c_str());
}

void insertClientAndDaemonTags(string NameClient, string NameDaemon, vector<string> C_SF,
		vector<string> C_RF, vector<string> D_SF, vector<string> D_RF)
{
	string optMergeInsertClientTagCommand, optMergeInsertDaemonTagCommand, optMergeCommandExtra;

	cout << endl << "Prepararing Client's .bc (" << NameClient << ")" << endl;

	optMergeInsertClientTagCommand = "opt -load " + MergePassPath
			+ " -Merge -FLG=1";

	optMergeCommandExtra = "";
	for (int a = 0; a < C_SF.size(); a++)
		optMergeCommandExtra += " -SF"
				+ static_cast<ostringstream*>(&(ostringstream() << a))->str()
				+ "=" + C_SF.at(a);
	for (int b = 0; b < C_RF.size(); b++)
		optMergeCommandExtra += " -RF"
				+ static_cast<ostringstream*>(&(ostringstream() << b))->str()
				+ "=" + C_RF.at(b);

	optMergeInsertClientTagCommand += optMergeCommandExtra + " <" + NameClient
			+ ".bc> ccc" + NameClient + ".bc";
	system_call((char*) optMergeInsertClientTagCommand.c_str());

	cout << endl << "Preparing Daemon's .bc (" << NameDaemon << ")" << endl;

	optMergeInsertDaemonTagCommand = "opt -load " + MergePassPath
			+ " -Merge -FLG=13";

	optMergeCommandExtra = "";
	for (int a = 0; a < D_SF.size(); a++)
		optMergeCommandExtra += " -SF"
				+ static_cast<ostringstream*>(&(ostringstream() << a))->str()
				+ "=" + D_SF.at(a);
	for (int b = 0; b < D_RF.size(); b++)
		optMergeCommandExtra += " -RF"
				+ static_cast<ostringstream*>(&(ostringstream() << b))->str()
				+ "=" + D_RF.at(b);

	optMergeInsertDaemonTagCommand += optMergeCommandExtra + " <" + NameDaemon
			+ ".bc> ttt" + NameDaemon + ".bc";
	system_call((char*) optMergeInsertDaemonTagCommand.c_str());
}

void linkClientAndDaemon(string NameClient, string NameDaemon)
{
	string llvmLinkCommand;

	cout << endl << "Linking Client's and Daemon's .bc (" << NameDaemon << "_" << NameClient << ")" << endl;

	llvmLinkCommand = "llvm-link ttt" + NameDaemon + ".bc ccc" + NameClient
			+ ".bc -o " + NameDaemon + "_" + NameClient + ".bc";
	system_call((char*) llvmLinkCommand.c_str());
}

void analyseUnifiedBC(string NameClient, string NameDaemon)
{
	string optNVACommand;

	cout << endl << "Analysing unified .bc (" << NameDaemon << "_" << NameClient << ")" << endl;

	optNVACommand = "opt -mem2reg -load " + PADriverPassPath + " -load "
            + AliasSetPassPath + " -load "
            + NetLevelPassPath + " -OutputDir=" + NameDaemon + "_" + NameClient + "/Graphs/ -stats -load " + NetDepGraphPassPath
			+ " -netVulArrays -BC=" + NameDaemon + "_" + NameClient + " "
            + graphFlag + " -debug <" + NameDaemon + "_" + NameClient
//            + graphFlag + " <" + NameDaemon + "_" + NameClient
			+ ".bc > " + NameDaemon + "_" + NameClient + "_net.txt -o bla2 2>&1";
	system_call((char*) optNVACommand.c_str());
}

void verifyResults(string NameClient, string NameDaemon)
{
	string grepCommand;
	string checkFileName = NameDaemon + "_" + NameClient + "_check.txt";

	cout << endl << "Veryfing results for " << NameDaemon << "_" << NameClient << endl;

	grepCommand = "grep \"Net Read\" " + NameDaemon + "_" + NameClient + "_netdepgraph.dot > " + NameDaemon + "_" + NameClient + "_check.txt";
	system((char*) grepCommand.c_str());


	ifstream in(checkFileName.c_str(), ifstream::in | ifstream::binary);
	in.seekg(0, ifstream::end);

	if (in.tellg() > 0)
		cout << endl << "Net nodes were found for " << NameDaemon << "_" << NameClient << endl << endl;
	else
		cout << endl << "No net node were found for " << NameDaemon << "_" << NameClient << endl << endl;
}

void postAnalysis(string NameClient, string NameDaemon)
{
	string mkdirCommand, mvCommand, rmCommand, statsCommand;

	mkdirCommand = "mkdir -p " + NameDaemon + "_" + NameClient;
	system((char*) mkdirCommand.c_str());

	mkdirCommand = "mkdir -p " + NameDaemon + "_" + NameClient + "/Graphs";
	system((char*) mkdirCommand.c_str());

	mvCommand =	"mv " + NameDaemon + "_" + NameClient
					+ ".bc " + NameDaemon + "_Daemonfuncnames.txt " + NameClient + "_Clientfuncnames.txt " + NameDaemon + "_" + NameClient + "_netsaved.txt "
					+ NameDaemon + "_" + NameClient + "_net.txt debugArq1 "
					+ NameDaemon + "_" + NameClient + "/ 2> /dev/null";
	system((char*) mvCommand.c_str());

	mvCommand =	"mv " + NameDaemon + "_" + NameClient + "_netdepgraph.dot " + NameDaemon + "_" + NameClient + "_sendRecvConnections.dot " + NameDaemon + "_" + NameClient + "_MemNodesP1-InputDepsP1Focus.dot " + NameDaemon + "_" + NameClient + "_MemNodesP1-NetInputDepsP1Focus.dot ";
	mvCommand += NameDaemon + "_" + NameClient + "_MemNodesP1-InputDepsP2Focus.dot " + NameDaemon + "_" + NameClient + "_MemNodesP2-InputDepsP2Focus.dot " + NameDaemon + "_" + NameClient + "_MemNodesP2-NetInputDepsP2Focus.dot ";
	mvCommand += NameDaemon + "_" + NameClient + "_MemNodesP2-InputDepsP1Focus.dot ";
	mvCommand += NameDaemon + "_" + NameClient + "_sendRecvConnectionsWithMemAndInputNodes.dot ";
	mvCommand += NameDaemon + "_" + NameClient + "/Graphs/ 2> /dev/null";
	system((char*) mvCommand.c_str());

	rmCommand = "rm " + NameDaemon + "_" + NameClient + "_check.txt ";

	rmCommand += "ccc" + NameClient + ".bc ttt" + NameDaemon
							+ ".bc";
	system((char*) rmCommand.c_str());

	cout << endl << endl << "NetVulArrays Statistics: " << endl << endl;

	statsCommand = "cp NetVulArrays_statistics.txt NetVulArrays_statistics.xls";
	statsCommand = "cat NetVulArrays_statistics.txt";

	system((char*) statsCommand.c_str());

	cout << endl << endl << "NetDepGraph Statistics: " << endl << endl;

	statsCommand = "cp NetDepGraph_statistics.txt NetDepGraph_statistics.xls";
	statsCommand = "cat NetDepGraph_statistics.txt";
	system((char*) statsCommand.c_str());
}

void postNoAnalysis(string NameClient, string NameDaemon)
{
	string mkdirCommand, mvCommand;

	mkdirCommand = "mkdir -p " + NameDaemon + "_" + NameClient;
	system_call((char*) mkdirCommand.c_str());

	mvCommand = "mv " + NameDaemon + "_Daemonfuncnames.txt " + NameClient + "_Clientfuncnames.txt "
			+ NameDaemon + "_" + NameClient + "/ 2> /dev/null";
	system_call((char*) mvCommand.c_str());
}

void *Analyse(void *threadid)
{
	string NameClient, NameDaemon;

	string go;

	vector<string> parameters(3), C_SF(4), C_RF(4), D_SF(4), D_RF(4);

	long threadID = (long)threadid;

	parameters = parametersList.at(threadID);
	C_SF = C_SFList.at(threadID);
	C_RF = C_RFList.at(threadID);
	D_SF = D_SFList.at(threadID);
	D_RF = D_RFList.at(threadID);

	go = parameters.at(0);
	NameClient = parameters.at(1);
	NameDaemon = parameters.at(2);

	if (go == "n" || go == "y")
	{
		cout << endl << "Starting... (" << NameDaemon << "_" << NameClient << ")" <<endl;

		findClientAndDaemonPossiblesSendRecv(NameClient, NameDaemon);
	}

	if (go == "y")
	{
        string mkdirCommand;

        mkdirCommand = "mkdir -p " + NameDaemon + "_" + NameClient;
        system((char*) mkdirCommand.c_str());

        mkdirCommand = "mkdir -p " + NameDaemon + "_" + NameClient + "/Graphs";
        system((char*) mkdirCommand.c_str());

		insertClientAndDaemonTags(NameClient, NameDaemon, C_SF, C_RF, D_SF, D_RF);

		linkClientAndDaemon(NameClient,NameDaemon);

		analyseUnifiedBC(NameClient, NameDaemon);

		verifyResults(NameClient, NameDaemon);

		postAnalysis(NameClient, NameDaemon);
	}

	if (go == "n")
	{
		postNoAnalysis(NameClient, NameDaemon);
	}



}

int main(int argc, char* argv[]) 
{	
	map<string,string> passPathAndFlags;

	if(openConfigFile(passPathAndFlags))
	{
		MergePassPath = passPathAndFlags["MergePassPath"];
		PADriverPassPath = passPathAndFlags["PADriverPassPath"];
		AliasSetPassPath = passPathAndFlags["AliasSetPassPath"];
        NetLevelPassPath = passPathAndFlags["NetLevelPassPath"];
		NetDepGraphPassPath = passPathAndFlags["NetDepGraphPassPath"];

		graphFlag = passPathAndFlags["graphFlag"];
		statsFlag = passPathAndFlags["statsFlag"];
		debugFlag = passPathAndFlags["debugFlag"];

		if(argc == 2 && openDataFile(argv[1],parametersList,C_SFList, C_RFList, D_SFList, D_RFList))
		{
			int numberOfThreads = parametersList.size();
			pthread_t threads[numberOfThreads];
			int rc;

			for(long t=0; t<numberOfThreads; t++)
			{
			    rc = pthread_create(&threads[t], NULL, Analyse, (void *)t);
			    if (rc)
			    {
			       printf("ERROR; return code from pthread_create() is %d\n", rc);
			       exit(-1);
			    }
			 }

			for(long t=0; t<numberOfThreads; t++)
			{
			  pthread_join(threads[t], NULL);
			}

			pthread_exit(NULL);

		}

	}


}

