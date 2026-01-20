#include <iostream>
#include <string>
#include <regex>
#include <cstdlib>
#include <cctype>
#include <fstream> 

//Copied from client.c
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
//


	
//using namespace std;
using std::cout;
using std::endl;
using std::string;
using std::regex;
using std::smatch;
using std::cmatch;
using std::regex_search;
using std::ofstream;




void parseCanonName(const string &userURL, string &canonName){
	string http_Identifier = "http://";
	int startIndex = http_Identifier.length();

	for(int i= startIndex; i < userURL.length(); i++)
		if(userURL[i] == ':' || userURL[i] == '/'){
			break;
		}
		else{
			canonName += userURL[i];
		}	
}



void parsePortNum(const string &userURL, int &currentIndex, string &portNum){
		currentIndex++;
		char currentElement = userURL[currentIndex];

		for(	; (userURL[currentIndex] != '/') && (currentIndex <= userURL.length()) ; currentIndex++){
			portNum += userURL[currentIndex];
		}
}



void parseFilePath(const string &userURL, int &currentIndex, string &filePath){
	for(	; currentIndex < userURL.length(); currentIndex++){
		filePath += userURL[currentIndex];
	}
}






//Take in sockaddr*,
//if IPv4, return reference to it's in_addr struct
//if IPv6, return reference to it's in6_addr struct
//return type here is void* (address pointing to ANY type)
	//Caller can cast it to correct type
void *get_in_addr(struct sockaddr *sa)
{
	if (sa->sa_family == AF_INET) {
		return &( ((struct sockaddr_in*)sa) -> sin_addr);
	}

	return &( ((struct sockaddr_in6*)sa) -> sin6_addr);
}






int main(int argc, char* argv[]){

	//Open a file, MyFile, to write to 
	ofstream MyFile("output");


    string userURL;
	// Convert argv[1] (user input URL), which is a char*, to a string userURL
	if(argc == 2){
		userURL = string(argv[1]);
	}



	//Series of tests to check functionality of pulling canonName from userURL
	//string userURL = "htstp://apple.com:80";	
	//string userURL = "http://apple.com:80";	
	//string userURL = "http://www.apple.com:80";	
	//string userURL = "http://apple.com";	
	//string userURL = "http://localhost:8888/path/to/file.html";	
	//string userURL = "http://localhost/path/to/file.html";		
	//string userURL = "http://nohosthere/path/to/file.html";	

	//const string userURL = "http://localhost:80/path/to/file.html";	

	//string userURL = "http://localhost:8000/serverExampleFile.txt";	
	//string userURL = "http://info.cern.ch/hypertext/WWW/TheProject.html";




	//regex expression for checking if userURL is 'http(s)://' followed by ANY sequence of characters
	//string http = R"(https?://[-A-Za-z0-9_.%]*)";
	string http = R"(http://.*)";
	regex http_expr(http);

	//Check to see if userURL contains http or https protocol 
	if( !regex_match(userURL, http_expr) ){
		//write "INVALIDPROTOCOL" to output file if protocol is NOT http://
		MyFile << "INVALIDPROTOCOL";
		MyFile.close();
		exit(0);
	}


	//Pulling canonName from userURL
	string canonName = "";
	parseCanonName(userURL, canonName);
	//cout << "canonName: " << canonName << endl;



	//Pulling Port # from userURL if there is an, ':', after canonName
	int startIndexOfCanonName = userURL.find(canonName);
	int currentIndex = startIndexOfCanonName + canonName.length();

	string portNum = "";
	if(userURL[currentIndex] == ':'){
		parsePortNum(userURL, currentIndex, portNum);
	}
	//cout << "portnum: " << portNum << endl;


	
	//Pulling file Path from userURL
	string filePath = "";
	if(userURL[currentIndex] == '/'){
		parseFilePath(userURL, currentIndex, filePath);
	}
	//cout << "filePath: "  << filePath << endl;








	//Now that we have finished parsing the URL
	//We will generate an LL of addrinfo structs, and attempt to make a socket and connect
	int sockfd;
	struct addrinfo hints, *servinfo, *p;
	int rv;

	//Create 1st addrinfo struct for LL, (IPv4 or IPv6), (TCP - Stream Sockets)
	memset(&hints, 0, sizeof hints);
	hints.ai_family = AF_UNSPEC;
	hints.ai_socktype = SOCK_STREAM;





	//Generate LL of addrinfo structs (for hostname passed in as command-line arg, Port 3490, store pointer to LL in servinfo)
	//print error message if getaddrinfo() call fails and return 1;
	
	//Make portNum = 80 if there was none specified in the userURL
	portNum = portNum.length() == 0 ? "80" : portNum;
	
	//Cast canonName and portNum to char*'s to be compatible with getaddrinfo()
	const char *canonicalName = canonName.c_str();
	const char *portNumber = portNum.c_str();



	if ((rv = getaddrinfo(canonicalName, portNumber, &hints, &servinfo)) != 0) {
		//May Need to change the below!!
		MyFile << "NOCONNECTION";
		MyFile.close();
		exit(0);
	}



	// p is an addrinfo*, iterates through the addrinfo structs in the LL until the end	
	// loop through the addrinfo structs and connect to THE FIRST ONE WE CAN:	
		// make LOCAL socket # 
		// make connection b/w LOCAL socket we just made, and DESTINATION address (IP/Port #) in the addrinfo struct 
	for(p = servinfo; p != NULL; p = p->ai_next) {
		if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
			perror("client: socket");
			continue;
		}

		//MAYBE use setsockopt if it becomes necessary
		//if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
		//	perror("setsockopt");
		//	exit(1);
		//}
	

		if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
			close(sockfd);
			perror("client: connect");
			continue;
		}

		break;
	}


	// If we made it through the ENTIRE LL and DID NOT connect() to ANY of the Destination addresses, p will be equal to NULL
	// print error that we failed to connect to any 
	if (p == NULL) {
		MyFile << "NOCONNECTION";
		MyFile.close();
		exit(0);
	}



	// get_in_addr returns reference to an (in_addr/in6_addr) struct
	// data from in_addr/in6_addr struct is copied to char[] s
	// print "connecting to " client address 
	char s[INET6_ADDRSTRLEN];

	inet_ntop(p->ai_family, get_in_addr((struct sockaddr *)p->ai_addr), s, sizeof s);
	//cout << "\nclient: connecting to " << s << "\n" << endl;


	// FREE up the LL!!
	freeaddrinfo(servinfo);







	//Now that we have a connection to the destination socket, we will generate the (const void *msg) GET request
	string header = "GET ";
	header += filePath += " HTTP/1.0\r\n\r\n";
	const char *getRequest = header.c_str();
	//cout << "getRequest: " << getRequest << endl;
	



	//Watch out for this to see if the num bytes is correct 
	//cout << "**len of getRequest: " << strlen(getRequest) << endl;


	//Now we send our http GET request
	if (send(sockfd, getRequest, strlen(getRequest), 0) == -1){
		perror("send");
		close(sockfd);
		exit(0);
	}

	


	//Now we need to receive the file
 	int MAXDATASIZE = 3000;
	int numbytes;
	char buf[MAXDATASIZE];

	//cout << "\n\n"; 
	int iternum = 0;


	bool lineOneProcessed = false;
	string lineOne;
	string responseCode;

	bool okayToWrite = false;
	int endOfHeaderIndex;

	int startIndex;

	while(true){
		//cout << "iternum: " << iternum++ << endl;

		//memset(buf, 0, sizeof(buf));
		buf[MAXDATASIZE] = {'\0'};

		if ( (numbytes = recv(sockfd, buf, MAXDATASIZE-1, 0) ) == -1) {
	    	//cout << "inside the IF-block\n";
			perror("recv");
	    	exit(1);
		}else if(numbytes == 0) {
			//cout << "inside the ELSE-IF block\n";
			//MyFile.close();
			break;
		}else{
			//cout << "inside the ELSE-block\n";

			if(lineOneProcessed == false){
				//parse line one from the response header
				string lineOneFormat = R"([a-zA-Z0-9_ /\.]*\r\n)";   
    			regex lineOneFormat_expr(lineOneFormat);
				cmatch m; 
			    regex_search(buf, m, lineOneFormat_expr); 
				lineOne = m[0].str();
				//cout << "lineOne: " << lineOne << endl;

				//parse the 3-digit response code from line one
				string responseCodeFormat = R"([0-9][0-9][0-9])";
    			regex response_expr(responseCodeFormat);
    			smatch m2; 
    			regex_search(lineOne, m2, response_expr); 
				responseCode = m2[0].str();
				//cout << "responseCode: " << responseCode << endl;

				if(responseCode == "404"){
					MyFile << "FILENOTFOUND";
					MyFile.close();
					exit(0);
				}
				lineOneProcessed = true;
			}
			
			if(okayToWrite == false){
				for(int i = 0; i < numbytes-3; i++){
					if(buf[i] == '\r' && buf[i+1] == '\n' && buf[i+2] == '\r' && buf[i+3] == '\n'){
						okayToWrite = true;
						endOfHeaderIndex = i+3;
						break;
					}
				}
			}
			
			if(okayToWrite == true){
				startIndex = endOfHeaderIndex >= 0 ? ++endOfHeaderIndex : 0;
				for(	; startIndex < numbytes; startIndex++){
					MyFile << buf[startIndex];
				}
				endOfHeaderIndex = -1;
			}



			//insert code here...

			/*
			for(int i = 0; i < numbytes; i++){
        		MyFile << buf[i];
    		}
			
			*/
			

			//MyFile << buf;
		}
		
	}
	

	MyFile.close();


	// Set the Null Terminator (\0) to the element IMMEDIATELY AFTER the last data element read in from recv() 
	//buf[numbytes] = '\0';


	// Print the data received from the client
	//cout << "\n\n\tClient Received: " << endl;
	//cout << buf << endl;


	// close BOTH reads and writes to our LOCAL socket (sockfd)
	close(sockfd);

	



	//cout << __cplusplus << endl;

	return 0;
}






