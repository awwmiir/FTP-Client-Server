#include "server.h"

void Server::printSyntaxError(std::string command, int commandSocket, int dataSocket, std::string username){
  logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", had a synatx error in using '"<<command<<"' command."<<std::endl;
  printTime();
}

void Server::printLoginError(std::string command, int commandSocket, int dataSocket){
  logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", used '"<<command<<"' without logging in."<<std::endl;
  printTime();
}

void Server::parse_json(){
  jsonParser();
  users_list = userGetter();
  protected_files = filesGetter();
  bool exists = file_exists("log.txt");
  logs.open("log.txt", std::ios_base::app);
  if(!exists){
    logs<<"Users:"<<std::endl;
    for(int i = 0; i < users_list.size(); i++){
        logs<<"\tusername: "<<users_list[i].get_username()<<std::endl;
        logs<<"\tpassword: "<<users_list[i].get_password()<<std::endl;
        logs<<"\tadmin: "<<(users_list[i].is_admin())? "true" : "false";logs<<std::endl;
        logs<<"\tsize: "<<users_list[i].get_size()<<std::endl;
        logs<<"******************************************"<<std::endl;
        
    }
    logs<<"Files:"<<std::endl;
    for(int i = 0; i < protected_files.size(); i++)
        logs<<"\t"<<protected_files[i]<<std::endl;
    logs<<"******************************************"<<std::endl;
  }
}

std::vector<std::string> Server::parse_command(char command[]){
    std::vector<std::string> parsed;
    std::string str;
    for(int i = 0; i < strlen(command); i++){
      if(command[i] == ' ' || command[i] == '\n'){
        parsed.push_back(str);
        str = "";
        continue;
      }
      str += command[i];
    }
    if(str != "")
      parsed.push_back(str);
    return parsed;
}

void Server::printTime(){
  std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
  logs<<"Time:\t"<<std::ctime(&now);
  logs<<"******************************************"<<std::endl;
}

socketData Server::handleIncomingConnections(){
  int reqSocket;
  int dataSocket;
  int addrlen;
  int addrlen2;
  struct sockaddr_in reqaddress;
  memset(&reqaddress, 0, sizeof(reqaddress));
  struct sockaddr_in dataaddress;
  memset(&dataaddress, 0, sizeof(dataaddress));
  addrlen = sizeof(reqaddress);
  addrlen2 = sizeof(dataaddress);
  if ((reqSocket= accept(request_channel, (struct sockaddr *)&reqaddress, (socklen_t*)&addrlen)) < 0){
      std::cout<<"accept"<<std::endl;
      exit(EXIT_FAILURE);
  }
  char* addr = inet_ntoa(reqaddress.sin_addr);
  char port[256]; 
  itoa_simple(port, ntohs(reqaddress.sin_port));
  std::string msg  = "$YOU ARE NOW CONNECTED TO REQUEST CHANNEL";
  logs<<"New Connection in request channel, address :127.0.0.1:"<<port<<std::endl;
  send(reqSocket, str_to_charstar(msg), 42, 0);
  if((dataSocket = accept(data_channel, (struct sockaddr *)&dataaddress, (socklen_t*)&addrlen2)) < 0){
      std::cout<<"accept"<<std::endl;
      exit(EXIT_FAILURE);
  }
  addr = inet_ntoa(dataaddress.sin_addr);
  port[256]; 
  itoa_simple(port, ntohs(dataaddress.sin_port));
  msg  = "$YOU ARE NOW CONNECTED TO DATA CHANNEL";
  logs<<"New Connection in data channel, address :127.0.0.2:"<<port<<std::endl;
  send(dataSocket, str_to_charstar(msg), 39, 0);
  int t, i;
  socketData newSocket;
  for (i = 0; i < CLIENT_COUNT; i++){
      if( clientSockets[i] == 0 && dataSockets[i] == 0){
          clientSockets[i] = reqSocket;
          dataSockets[i] = dataSocket;
          newSocket.commandSocket = reqSocket;
          newSocket.dataSocket = dataSocket;
          logs<<"Adding to list of sockets as: "<<i<<std::endl;
          printTime();
          break;
      }
  }
  return (newSocket);
}

void Server::connectChannels(char* ports[]){
    if((request_channel = socket(AF_INET, SOCK_STREAM, 0)) < 0 
        || (data_channel = socket(AF_INET, SOCK_STREAM, 0 )) < 0)
    {
    std::cout<<"socket() failed"<<std::endl;
    exit(EXIT_FAILURE);
    }
    int opt = 1, opt2 = 1;
    if(setsockopt(request_channel, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char *)&opt, sizeof(opt)) < 0
        || setsockopt(data_channel, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, (char *)&opt2, sizeof(opt2)) < 0)
    {
    std::cout<<"setsockopt() failed"<<std::endl;
    exit(EXIT_FAILURE);
    }

    memset(&request_serverAddr, 0, sizeof(request_serverAddr));
    memset(&data_serverAddr, 0, sizeof(data_serverAddr));

    request_serverAddr.sin_family = AF_INET;
    request_serverAddr.sin_port = htons(atoi(ports[0]));
    request_serverAddr.sin_addr.s_addr = inet_addr(REQUEST_ADDR);

    data_serverAddr.sin_family = AF_INET;
    data_serverAddr.sin_port = htons(atoi(ports[1]));
    data_serverAddr.sin_addr.s_addr = inet_addr(DATA_ADDR);
    if (bind(request_channel, (struct sockaddr *) &request_serverAddr, sizeof(request_serverAddr)) < 0
        || bind(data_channel, (struct sockaddr *) &data_serverAddr, sizeof(data_serverAddr)) < 0)
    {
      std::cout<<"bind() failed"<<std::endl;
      exit(EXIT_FAILURE);
    }
    if(listen(request_channel, CLIENT_COUNT) < 0
        || listen(data_channel, CLIENT_COUNT) < 0){
      std::cout<<"listen() failed"<<std::endl;
      exit(EXIT_FAILURE);
    }
}

bool Server::find_username(std::string username){
  for(int i = 0; i < users_list.size(); i++)
    if(users_list[i].get_username() == username)
      return true;
  return false;
}

bool Server::find_password(std::string password, std::string username, bool* isAdmin){
  for(int i = 0; i < users_list.size(); i++)
    if(users_list[i].get_password() == password && users_list[i].get_username() == username){
      *isAdmin = users_list[i].is_admin();
      return true;
    }
  return false;
}

void Server::handle_user(std::string* loggedInUsername, bool* user, bool pass, int commandSocket, int dataSocket, std::vector<std::string> parsed){
  if(*user && pass){
    send(commandSocket, "500: Error", 12, 0);
    logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", tried to change username when logged in."<<std::endl;
    printTime();
    return;
  }
  if(parsed.size() != 2){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("user", commandSocket, dataSocket, *loggedInUsername);
    *user = false;
    *loggedInUsername = "";
  }
  else{
    if(find_username(parsed[1])){
      send(commandSocket, "331: User name okay, need password.", 36, 0);
      *user = true;
      *loggedInUsername = parsed[1];
      logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", entered username: "<<parsed[1]<<", should enter password."<<std::endl;
      printTime();
    }
    else{
      send(commandSocket, "430: Invalid username or password.", 35, 0);
      *user = false;
      *loggedInUsername = "";
      logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", entered an invalid username."<<std::endl; 
      printTime();
    }
  }
}

void Server::handleIncomingInformation(void* newSocket){
  bool user = false, pass = false;
  socketData * sock = (socketData *)newSocket;
  std::string loggedInUsername, directory = "./server";
  bool isAdmin = false;
  while(1){
      char * in = new char[256];
      recv(sock->commandSocket, in, 256, 0);
      std::vector<std::string> parsed = parse_command(in);
      if(parsed[0] == "user")
        handle_user(&loggedInUsername, &user, pass, sock->commandSocket, sock->dataSocket, parsed);
      else if(parsed[0] == "pass")
        handle_pass(loggedInUsername, &user, &pass, sock->commandSocket, sock->dataSocket, parsed, &isAdmin);
      else if(parsed[0] == "help")
        handle_help(parsed, sock->commandSocket, sock->dataSocket, user, pass, loggedInUsername);
      else if(parsed[0] == "pwd")
        handle_pwd(parsed, directory, sock->commandSocket, sock->dataSocket, user, pass, directory, loggedInUsername);
      else if(parsed[0] == "cwd")
        handle_cwd(parsed, sock->commandSocket, sock->dataSocket, user, pass, &directory, loggedInUsername);
      else if(parsed[0] == "mkd")
        handle_mkd(parsed, sock->commandSocket, sock->dataSocket, user, pass, directory, loggedInUsername);
      else if(parsed[0] == "dele")
        handle_dele(parsed, sock->commandSocket, sock->dataSocket, user, pass, directory, isAdmin, loggedInUsername);
      else if(parsed[0] == "retr")
        handle_dl(parsed, sock->commandSocket, sock->dataSocket, user, pass, isAdmin, directory, loggedInUsername);
      else if(parsed[0] == "ls")
        handle_ls(parsed, sock->commandSocket, sock->dataSocket, user, pass, directory, loggedInUsername);
      else if(parsed[0] == "rename")
        handle_rename(parsed, sock->commandSocket, sock->dataSocket, user, pass, isAdmin, directory, loggedInUsername);
      else if(parsed[0] == "quit")
        handle_quit(parsed, sock->commandSocket, sock->dataSocket, &user, &pass, &directory, &isAdmin, &loggedInUsername);
      else
        handle_error(sock->commandSocket, sock->dataSocket);
      delete in;
  }
}

void Server::handle_pass(std::string username, bool* user, bool* pass, int commandSocket, int dataSocket, std::vector<std::string> parsed, bool* isAdmin){
  if(*user && *pass){
    send(commandSocket, "500: Error", 12, 0);
    logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", tried to enter password when logged in."<<std::endl;
    printTime();
    return;
  }
  if(!(*user)){
    send(commandSocket, "503: Bad sequence of commands.", 31, 0);
    logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", entered password before entering username."<<std::endl;
    printTime();
    *pass = false;
    return;
  }
  if(parsed.size() != 2){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("pass", commandSocket, dataSocket, "");
    *pass = false;
    return;
  }
  else{
    if(find_password(parsed[1], username, isAdmin)){
      send(commandSocket, "230: User logged in, proceed. Logged out if appropriate.", 57, 0);
      logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<",logged in as: "<<username<<std::endl;
      *pass = true;
      
    }
    else{
      send(commandSocket, "430: Invalid username or password.", 35, 0);
      logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", entered an invalid password."<<std::endl; 
      *pass = false;
    }
    printTime();
  }
  
}

void Server::handle_help(std::vector<std::string> parsed, int commandSocket, int dataSocket, bool user, bool pass, std::string username){ 
  if(!user || !pass){
    send(commandSocket, "332: Need account for login.", 29, 0);
    printLoginError("help", commandSocket, dataSocket);
    return;
  }
  if(parsed.size() != 1){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("help", commandSocket, dataSocket, username);
    return;
  }
  send(commandSocket, "214", 4, 0);
  send_help(commandSocket);
  logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", used 'help' command.";
  printTime();

}

void Server::handle_pwd(std::vector<std::string> parsed, std::string directory,
 int commandSocket, int dataSocket, bool user, bool pass, std::string path, std::string username){
  if(!user || !pass){
    send(commandSocket, "332: Need account for login.", 29, 0);
    printLoginError("pwd", commandSocket, dataSocket);
    return;
  }
  if(parsed.size() != 1){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("pwd", commandSocket, dataSocket, username);
    return;
  }
  path = "257: " + path;
  send(commandSocket, path.c_str(), path.size(), 0);
  logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", used 'pwd' command"<<std::endl;
  printTime();
}

std::string Server::checkForServer(std::string cwd, bool* flag){
  *flag = false;
  int j = 0;
  std::string str;
  for(int i = 0; i < cwd.size(); i++){
    str += cwd[i];
    if(str == "./server"){
      *flag = true;
      j = i + 2;
      break;
    }
    else if( i > 7 )
      break;
  }
  if(*flag){
    str = "";
    for(int i = j; i < cwd.size(); i++)
      str += cwd[i];
    return str;
  }
  return cwd;
} 

void Server::handle_cwd(std::vector<std::string> parsed, int commandSocket, int dataSocket, bool user, bool pass, std::string* cwd, std::string username){
  if(!user || !pass){
    send(commandSocket, "332: Need account for login.", 29, 0);
    printLoginError("cwd", commandSocket, dataSocket);
    return;
  }
  if(parsed.size() < 1 || parsed.size() > 2){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("cwd", commandSocket, dataSocket, username);
    return;
  }
  if(parsed.size() == 1 || (parsed[1] == ".." && *cwd == "./server"))
    *cwd = "./server";
  else if(parsed[1] == ".." && *cwd != "./server")
    *cwd = move_back(*cwd);
  else{
    bool flag;
    std::string newCWD = checkForServer(parsed[1], &flag);
    std::string path = fix_addres(*cwd);
    path = get_working_path() + path + "/" + parsed[1];
    if(doesDirExist(get_working_path() + "/" + newCWD)){
      *cwd = "./server/" + newCWD;
    }
    else if(doesDirExist(path))
      *cwd = *cwd + "/" + parsed[1];
    else{
      send(commandSocket, "500: Error", 11, 0);
      logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was changing to an unavailable directory."<<std::endl;
      printTime();
      return;
    }
  }
  send(commandSocket, "250: Successful change.", 24, 0);
  logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", used 'cwd' command."<<std::endl;
  logs<<"new user directory: "<< *cwd<<std::endl;
  printTime();
}

void Server::handle_mkd(std::vector<std::string> parsed, int commandSocket, int dataSocket, bool user, bool pass, std::string cwd, std::string username){
  if(!user || !pass){
    send(commandSocket, "332: Need account for login.", 29, 0);
    printLoginError("mkd", commandSocket, dataSocket);
    return;
  }
  if(parsed.size() != 2){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("mkd", commandSocket, dataSocket, username);
    return;
  }
  bool flag;
  checkForServer(parsed[1], &flag);
  std::string command = "mkdir ";
  if(flag){
    std::string temp1 = get_working_path() + fix_addres(parsed[1]);
    std::string temp2 = get_working_path() + findDirectory(fix_addres(parsed[1]));
    if(doesDirExist(temp1) || !doesDirExist(temp2)){
      logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was either creating a directory that existed befor ";
      logs<<"or does not exist at all."<<std::endl;
      printTime();
      send(commandSocket, "500: Error", 11, 0);
      return;
    }
    command += temp1;
  }
  else{
    std::string temp1 = fix_addres(cwd);
    temp1 = get_working_path() + temp1 + "/" + parsed[1];
    std::string temp2 = fix_addres(cwd);
    temp2 = get_working_path() + temp2 + "/" + findDirectory(parsed[1]);
    if(doesDirExist(temp1) || !doesDirExist(temp2)){
      logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was either creating a directory that existed befor ";
      logs<<"or does not exist at all."<<std::endl;
      printTime();
      send(commandSocket, "500: Error", 11, 0);
      return;
    }
    command += temp1;
  }
  system(command.c_str());
  std::string msg = "257: " + (flag ? "./server" : cwd) + "/" + checkForServer(parsed[1], &flag) + " created.";
  send(commandSocket, msg.c_str(), msg.size(), 0);
  logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" created "<<msg<<std::endl;
  printTime();
}

void Server::handle_dele(std::vector<std::string> parsed, int commandSocket, int dataSocket, bool user, bool pass, std::string cwd, bool isAdmin, std::string username){
  if(!user || !pass){
    send(commandSocket, "332: Need account for login.", 29, 0);
    printLoginError("dele", commandSocket, dataSocket);
    return;
  }
  if(parsed.size() != 3){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("dele", commandSocket, dataSocket, username);
    return;
  }
  if(parsed[1] == "-d"){
    std::string path = fix_addres(cwd);
    path = get_working_path() + path + "/" + parsed[2];
    if(!doesDirExist(path)){
      logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was deleting a directory that didn't exist."<<std::endl;
      printTime();
      send(commandSocket, "500: Error", 11, 0);
      return;
    }
    std::string comm = "rm -r " + path;
    system(comm.c_str());
    std::string msg = "250: " + cwd + "/" + parsed[2] + " deleted.";
    send(commandSocket, msg.c_str(), msg.size(), 0);
    logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", deleted directory: "<<cwd<<"/"<<parsed[2]<<std::endl;
    printTime();
  }
  else if(parsed[1] == "-f"){
    std::string path = fix_addres(cwd);
    path = get_working_path() + path + "/" + parsed[2];
    if(!file_exists(path.c_str())){
      logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was deleting a file that didn't exist."<<std::endl;
      printTime();
      send(commandSocket, "500: Error", 11, 0);
      return;
    }
    if(std::count(protected_files.begin(), protected_files.end(), findFileName(parsed[2])) > 0 && !isAdmin){
      logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was deleting a file that was inaccessible."<<std::endl;
      printTime();
      send(commandSocket, "550: File unavailable.", 23, 0);
      return;
    }
    unlink(path.c_str());
    std::string msg = "250: " + cwd + "/" + parsed[2] + " deleted.";
    send(commandSocket, msg.c_str(), msg.size(), 0);
    logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", deleted file: "<<cwd<<"/"<<parsed[2]<<std::endl;
    printTime();
  }
  else{ 
    send(commandSocket, "500: Error", 11, 0);
    logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<"had an error in using 'dele' command."<<std::endl;
    printTime();
  }
}

void Server::handle_dl(std::vector<std::string> parsed, int commandSocket, int dataSocket, bool user, bool pass, bool isAdmin, std::string cwd, std::string loggedInUsername){
  if(!user || !pass){
    send(commandSocket, "!332: Need account for login.", 30, 0);
    printLoginError("ls", commandSocket, dataSocket);
    return;
  }
  else if(parsed.size() != 2){
    send(commandSocket, "!501: Syntax error in parameters or arguments.", 47, 0);
    printSyntaxError("ls", commandSocket, dataSocket, loggedInUsername);
    return;
  }
  std::string path = get_working_path() + fix_addres(cwd);
  path += "/" + parsed[1];
  if(!file_exists(path.c_str())){
      logs<<"client "<<loggedInUsername<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was downloding a file that didn't exist."<<std::endl;
      printTime();
      send(commandSocket, "!500: Error", 12, 0);
      return;
  }
  std::string file = findFileName(parsed[1]);
  if(std::count(protected_files.begin(), protected_files.end(), file) > 0 && !isAdmin){
    logs<<"client "<<loggedInUsername<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was downloading a file that was inaccessible."<<std::endl;
    printTime();
    send(commandSocket, "!550: File unavailable.", 24, 0);
    return;
  }

  std::ifstream ifs(file);
  std::string content( (std::istreambuf_iterator<char>(ifs) ),
                       (std::istreambuf_iterator<char>()    ) );
  ifs.close();

  for(int i = 0; i < users_list.size(); i++) {
    if(users_list[i].get_username() == loggedInUsername) {
      int remaining_size = users_list[i].get_size();
      int content_size = content.size();
      if((remaining_size - content_size) < 0) {
        logs<<"client "<<loggedInUsername<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was downloading a too large file."<<std::endl;
        printTime();
        send(commandSocket, "!425: Can't open data connection.", 34, 0);
        return;
      } 
      else {
        users_list[i].change_size(content.size());
        break;
      }
    }
  }

  send(commandSocket, "#226: Successfull Download.", 28, 0);
  send(dataSocket, content.c_str(), content.size()+1, 0);
  logs<<"client "<<loggedInUsername<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", used 'retr " << parsed[1] <<"' command"<<std::endl;
  printTime();
}

void Server::handle_ls(std::vector<std::string> parsed, int commandSocket, int dataSocket, bool user, bool pass, std::string cwd, std::string username){
  if(!user || !pass){
    send(commandSocket, "!", 2, 0);
    send(commandSocket, "332: Need account for login.", 29, 0);
    printLoginError("ls", commandSocket, dataSocket);
    return;
  }
  if(parsed.size() != 1){
    send(commandSocket, "!", 2, 0);
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("ls", commandSocket, dataSocket, username);
    return;
  }

  std::string path = get_working_path() + fix_addres(cwd);
  DIR *dir;
  struct dirent *ent;
  if ((dir = opendir (path.c_str())) != NULL) {
    send(commandSocket, "#", 2, 0);
    std::string str = "";
    while ((ent = readdir (dir)) != NULL){
      if(strcmp(ent->d_name, "..")==0 || strcmp(ent->d_name, ".") == 0)
        continue;
      str += ent->d_name;
      str += "\n";
    }
    send(dataSocket, str.c_str(), str.size()+1, 0);
    send(commandSocket, "226: List transfer done.", 25, 0);
    logs<<"client "<<username<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", used 'ls' command"<<std::endl;
    printTime();
    closedir (dir);
  }
}

void Server::handle_quit(std::vector<std::string> parsed, int commandSocket, int dataSocket, bool* user, bool* pass, std::string* directory,
                   bool* isAdmin, std::string *loggedInUsername){
  if(!user || !pass){
    send(commandSocket, "332: Need account for login.", 29, 0);
    printLoginError("quit", commandSocket, dataSocket);
    return;
  }
  if(parsed.size() != 1){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("quit", commandSocket, dataSocket, *loggedInUsername);
    return;
  }
  logs<<"client "<<*loggedInUsername<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", used 'quit' command"<<std::endl;
  printTime();
  *user = false; *pass = false; *isAdmin = false;
  *directory = "./server"; *loggedInUsername = "";
  send(commandSocket, "221: Successful Quit.", 22, 0);
}

void Server::send_help(int commandSocket){
  std::string str = "1-USER <username>:\nUsed for authentication, gets the username as input and if it is found gives <331:Username okay, need password> as output.";
  str += "\n\n2-Pass <password>:\nafter inserting the username, the password is required to login to your account.";
  str += "\n\n3-pwd:\nused to get the current working directory path.";
  str += "\n\n4-mkd <directory path>:\nmake a neew directory in the given path.";
  str += "\n\n5-dele -f <filename>:\nremove a file in the current working directory.";
  str += "\n\n6-dele -d <directory path>:\ndelete a given directory.";
  str += "\n\n7-ls:\nlist all the files in the current working directory.";
  str += "\n\n8-cwd <path>:\nchange the working directory.\nwith \"..\" given as input you can move to the last directory.\ngiven no input as argument, you can go back to the first directory.";
  str += "\n\n9-rename <from> <to>:\nchange the name of a file in the current working directory.";
  str += "\n\n10-retr <name>:\nyou can download a file in the current working directory if you have the required data size.";
  str += "\n\n11-help:\nlists all the possible commands with their manual.";
  str += "\n\n12-quit:\nlogout of the system.";
  send(commandSocket, str.c_str(), str.size(), 0);
}

void Server::handle_rename(std::vector<std::string>parsed, int commandSocket, int dataSocket, bool user, bool pass, bool isAdmin, std::string cwd, std::string loggedInUsername){
  if(!user || !pass){
    send(commandSocket, "332: Need account for login.", 29, 0);
    printLoginError("rename", commandSocket, dataSocket);
    return;
  }
  if(parsed.size() != 3){
    send(commandSocket, "501: Syntax error in parameters or arguments.", 46, 0);
    printSyntaxError("rename", commandSocket, dataSocket, loggedInUsername);
    return;
  }
  std::string path = get_working_path() + fix_addres(cwd);
  path += "/" + parsed[1];
  if(!file_exists(path.c_str())){
      logs<<"client "<<loggedInUsername<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was renaming a file that didn't exist."<<std::endl;
      printTime();
      send(commandSocket, "500: Error", 11, 0);
      return;
  }
  std::string file = findFileName(parsed[1]);
  if(std::count(protected_files.begin(), protected_files.end(), file) > 0 && !isAdmin){
    logs<<"client "<<loggedInUsername<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" was renaming a file that was inaccessible."<<std::endl;
    printTime();
    send(commandSocket, "550: File unavailable.", 23, 0);
    return;
  }
  std::string command = "mv " + get_working_path() + fix_addres(cwd) +"/"+ parsed[1] + " " + get_working_path() + fix_addres(cwd)
   +"/"+ parsed[1].substr(0, parsed[1].find(findFileName(parsed[1]))) + findFileName(parsed[2]);
  system(command.c_str());
  send(commandSocket, "250: Successful change.", 24, 0);
  logs<<"client "<<loggedInUsername<<" with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<" renamed file: "<<cwd<<"/"<<parsed[1] << " to: "<<parsed[2]<<std::endl;
  printTime();
}

void Server::handle_error(int commandSocket, int dataSocket){
  send(commandSocket, "500: Error", 11, 0);
  logs<<"client with command socket id: "<<commandSocket<<", data socket id: "<<dataSocket<<", entered an unknown command."<<std::endl;
  printTime();
}

