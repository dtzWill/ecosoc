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


class InstrumentPoint{
	string file;
	vector<int> lines;
   public:
	InstrumentPoint(){}
	string getFile(){ return file;}
	void setFile(string file){ this->file = file;}
	vector<int> getLines(){return lines;}
	void addLine(int line){ this->lines.push_back(line);}
};

//FUNCTIONS

void system_call(char *cmd) 
{
	cout << "cmd=" << cmd << endl;
	system(cmd);
}

vector<InstrumentPoint> openFile(char *fileName)
{
	string line;

	ifstream file;
	vector<InstrumentPoint> instPointList; 
	file.open(fileName);
	if (!file.is_open()) {
		cout << "Could not open input file \n";
		exit(1);
	} else {
		cout << "Input file has opened \n";
		InstrumentPoint *previous = new InstrumentPoint();
		if(getline(file,line,' ')) previous->setFile(line);
		do{
		        if(previous->getFile()!=line){ //new file -> new IntrumentPoint
				instPointList.push_back(*previous);
				previous = new InstrumentPoint();
				previous->setFile(line);
				getline(file, line);
				previous->addLine(atoi(line.c_str()));
				cout << "New InstPoint created to file:["<<previous->getFile()<<" lines size:["<<previous->getLines().size()<<"]\n";
			}else{
				getline(file, line);
				previous->addLine(atoi(line.c_str()));
				cout << "Add Lines to InstPoint file:["<<previous->getFile()<<" lines size:["<<previous->getLines().size()<<"]\n";
			}
		} while (getline(file, line,' ')) ;
		instPointList.push_back(*previous);
	}
	file.close();
	for(vector<InstrumentPoint>::iterator p = instPointList.begin(), end = instPointList.end(); p != end; p++){
			cout << "Test openFile - file has opened:["<<p->getFile()<<"] abcs ["<<p->getLines().size()<<"]\n";
	}
	return instPointList;				
}
string abc(string fileName,int line, int index){
	stringstream s; 
	//s << "  // ABC - Begin - Array Bound Check \n int index = 10;\n if(index < 0 || index > 100){ \n PRINTF(\"\\nMessage out of bounds);\n return;\n }else{\n // Array Bounds are ok, then do nothing\n }\n //ABC - END\n\n ";
	//return "  // ABC - Begin - Array Bound Check \n int index = 10;\n if(index < 0 || index > 100){ \n PRINTF(\"\\nMessage out of bounds);\n return;\n }else{\n // Array Bounds are ok, then do nothing\n }\n //ABC - END\n\n ";*/
	//return "/* ABC */\n";
        cout << "ABC has inserted in File: " << fileName << " Line: "<< line <<" index:" << index <<"\n";
      	s << "//****** ABC test: ";
	s << "File: " << fileName << " Line: "<< line <<" index:" << index <<"******/\n";
        s << "int a_line_"<<line<<"_"<<index<<"[MAX_ARRAY_SIZE];\n";
        s << "int i_line_"<<line<<"_"<<index<<" =1;\n";
	s << "print_abc(\"" << fileName << "\" ," << line<< "," <<  index << ");\n";
	s << "add_abc();\n";
        s << "if(i_line_"<<line<<"_"<<index<<"<0 || i_line_"<<line<<"_"<<index<<" >MAX_ARRAY_SIZE){\n";
        s << "        abc_handler();\n";
        s << " } else {\n";
        s << "        a_line_"<<line<<"_"<<index<<"[i_line_"<<line<<"_"<<index<<"] = 1;\n";
        s << " }\n";
        s << " //****** ABC test **********/\n";
	return s.str();
}


int instrument(vector<InstrumentPoint> i){
    int abcs =0;
    for(vector<InstrumentPoint>::iterator p = i.begin(), end = i.end(); p != end; p++){
	string fileName = p->getFile();
	fstream file;
	file.open(fileName.c_str());
	if (!file.is_open()) {
		cout << "Could not open the input file["<< fileName <<"]\n";
		exit(1);
	} else {
   	     cout << "Input file has opened:["<<fileName<<"] abcs ["<<p->getLines().size()<<"]\n";
	     string tmpFileName = fileName + ".tmp";
	     ofstream tmpfile(tmpFileName.c_str());
	     vector<int> lines = p->getLines();
	     bool begin = true;
	     int lineNumber=0;
	     int insertLineNumber=0;
	     string line="";
	     int previousLine = -1;
	     int index=0;
	     bool endFile = false;
	     bool changeInsertLine = false;
	     bool inserted = false;
	     bool last = false;
	     for(vector<int>::iterator l = lines.begin(), end = lines.end(); l != end; l++) //Check all inst points
	     {
	         cout << "FileName[" << fileName <<"] ABC Line:["<<*l<<"] PreviousLine[" << previousLine << "]lineNumber["<<lineNumber<<"]\n"; 
		 last = false;
		 if(*l!=lineNumber && !endFile) {//Forward the file until target line 
		     cout <<"*l not equal to lineNumber\n";
		     while(lineNumber < insertLineNumber) lineNumber++; //Forward the lineNumber until insertLineNumber	
		         index = 0;
			 if(begin) {
			     tmpfile << "#include \"siot.h\"" << endl;
			     begin =false;
			 }
/*					else {
						tmpfile << line << "\n";
					}
*/
			lineNumber++;
			while (getline(file, line) && lineNumber < (*l)) {
			    cout << "Line without ABCs:["<<lineNumber<<"]\n"; 
			    tmpfile << line << "\n";
			    lineNumber++;
			}
			if(lineNumber < (*l)) {
			    endFile = true;
			    cout << "End of file lineNumber[" << lineNumber << "] ABC Line ["<< *l <<"]\n"; 	
			}
		     }
		     /*
			1st case - simple - include the abc
                        2nd case - search the correct insert point
			3rd case - insert abc in the same line - change the index
			4th case - end of file
		     */
/*		     insertLineNumber = lineNumber;
		     char line_end = line[line.length()-1];
		     while(line_end !=';' && line_end != ' ' && !endFile) { //Search for the correct point to insert the ABC
		         cout << "Search for the correct point to insert the ABC - LineNumber[" << lineNumber << "] InsertLineNumber ["<< insertLineNumber << "] line_end [" << line_end<< "]\n";
//					//if(end !=';' && end !='{') {
//					//if(end =='{' || end == ',' || end == '') {
						//tmpfile << line << "\n";
						if(!getline(file,line)) { 
							endFile = true;
							cout << "End of file lineNumber[" << lineNumber << "] ABC Line ["<< *l <<"] insertLineNumber ["<< insertLineNumber< "]\n"; 	
							break;
						}
						insertLineNumber++;
						line_end = line[line.length()-1];
						changeInsertLine = true;
						inserted = false;
		     }
*/		     if(previousLine == *l ) {
//		     if(previousLine == *l || endFile) {
			index++; // New ABC in the same line -> increment the index
			last = true;
		     }
		     else {
/*						if(changeInsertLine){
							cout << "Insert line after index++: " << line << "\n";
							tmpfile << line << "\n";
						}
*/
/*		         if (!changeInsertLine) {
				cout << "Insert line after index++: " << line << "\n";
				tmpfile << line << "\n";
			 }

			 changeInsertLine = false;
			 inserted = false;
*/
				tmpfile << line << "\n";
		     }
		     tmpfile << abc(fileName, *l,index);
		     abcs++;
//					}
		     previousLine = *l;
				
		}
		do{
/*			if(last) {
				getline(file,line);
				lineNumber++;
				last = false;
			}
*/			tmpfile << line << "\n";
			cout << "Line without ABCs in the final of file:["<<lineNumber<<"]\n"; 
			lineNumber++;
		} while (getline(file, line));
/*			while (getline(file, line)){
				tmpfile << line << "\n";
			} 
*/		tmpfile.close();
	}
    }
    return abcs;
}
/*
int instrument(vector<InstrumentPoint> i){
	int abcs =0;
	for(vector<InstrumentPoint>::iterator p = i.begin(), end = i.end(); p != end; p++){
		string fileName = p->getFile();
		fstream file;
		file.open(fileName.c_str());
		if (!file.is_open()) {
			cout << "Could not open the input file["<< fileName <<"]\n";
			exit(1);
		} else {
			cout << "Input file has opened:["<<fileName<<"] abcs ["<<p->getLines().size()<<"]\n";
			string tmpFileName = fileName + ".tmp";
			ofstream tmpfile(tmpFileName.c_str());
			vector<int> lines = p->getLines();
			bool begin = true;
			int lineNumber=0;
			int insertLineNumber=0;
			string line="";
			int previousLine = -1;
			int index=0;
			bool endFile = false;
			bool changeInsertLine = false;
			bool inserted = false;
			for(vector<int>::iterator l = lines.begin(), end = lines.end(); l != end; l++) //Check all inst points
			{
				cout << "FileName[" << fileName <<"] ABC Line:["<<*l<<"] PreviousLine[" << previousLine << "]lineNumber["<<lineNumber<<"]\n"; 
				if(*l!=lineNumber && !endFile) {//Forward the file until target line 
					cout <<"*l not equal to lineNumber\n";
				        while(lineNumber < insertLineNumber) lineNumber++; //Forward the lineNumber until insertLineNumber	
					index = 0;
					if(begin) {
						tmpfile << "#include \"siot.h\"" << endl;
						begin =false;
					}
					lineNumber++;
					while (getline(file, line) && lineNumber < (*l)) {
						cout << "Line without ABCs:["<<lineNumber<<"]\n"; 
						tmpfile << line << "\n";
						lineNumber++;
					}
					if(lineNumber < (*l)) {
						endFile = true;
						cout << "End of file lineNumber[" << lineNumber << "] ABC Line ["<< *l <<"]\n"; 	
					}
				}
				cout <<"*l equal to previousLine\n";
				insertLineNumber = lineNumber;
				char line_end = line[line.length()-1];
					while(line_end !=';' && line_end != ' ' && !endFile) { //Search for the correct point to insert the ABC
						cout << "Search for the correct point to insert the ABC - LineNumber[" << lineNumber << "] InsertLineNumber ["<< insertLineNumber << "] line_end [" << line_end<< "]\n";
//					//if(end !=';' && end !='{') {
//					//if(end =='{' || end == ',' || end == '') {
						//tmpfile << line << "\n";
						if(!getline(file,line)) { 
							endFile = true;
							cout << "End of file lineNumber[" << lineNumber << "] ABC Line ["<< *l <<"] insertLineNumber ["<< insertLineNumber< "]\n"; 	
							break;
						}
						insertLineNumber++;
						line_end = line[line.length()-1];
						changeInsertLine = true;
						inserted = false;
					}
//					} else {
					    if(previousLine == *l || endFile) index++; // New ABC in the same line -> increment the index
					        if (!changeInsertLine) {
							cout << "Insert line after index++: " << line << "\n";
							tmpfile << line << "\n";
						}
						changeInsertLine = false;
						inserted = false;
					    }
					    tmpfile << abc(fileName, *l,index);
					    abcs++;
//					}
				previousLine = *l;
				
			}
			do{
				tmpfile << line << "\n";
			} while (getline(file, line));
			tmpfile.close();
		}
	}
	return abcs;
}
*/
int main(int argc, char* argv[]) 
{	

	vector<InstrumentPoint> instPointList;
	instPointList = openFile(argv[1]);
	for(vector<InstrumentPoint>::iterator p = instPointList.begin(), end = instPointList.end(); p != end; p++){
			cout << "Test main - file has opened:["<<p->getFile()<<"] abcs ["<<p->getLines().size()<<"]\n";
	}
	int abcs = instrument(instPointList);
	cout << abcs << " ABCs have been inserted";
}
