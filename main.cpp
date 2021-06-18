#include <windows.h>
#include <stdio.h>
#include <shlwapi.h>

#include <string>
#include <vector>
#include <algorithm>
#include <stdint.h>

#include <shellapi.h>
#include <stdlib.h>

//------------------------------------------------------------------------------
bool Processing(const uint32_t &Address, const uint32_t &Length);//��������� �� ����������
void ProcessingFile(const std::string &path,const std::string &mask,std::vector<std::string> &file_array);//���������
void FindFile(const std::string &path,const std::string &mask,std::vector<std::string> &file_array);//��������� ������
void FindPath(const std::string &path,const std::string &mask,std::vector<std::string> &file_array);//��������� ���������
DWORD Execute(const char *name,const char *param,const char *directory,bool use_stdout_and_stderror);//������ �� ����������
bool ProcessingCFiles(const std::string &path,const std::string &mask,const std::string &ac30_file_name,const std::string &include_file_name);//��������� c-������
bool ProcessingIFFiles(const std::string &path,const std::string &opt30_file_name);//��������� if-������
bool ProcessingOPTFiles(const std::string &path,const std::string &cg30_file_name);//��������� opt-������
bool ProcessingASMFiles(const std::string &path,const std::string &asm30_file_name);//��������� asm-������
bool ProcessingOBJFiles(const std::string &path,const std::string &lnk30_file_name,const std::string &libs_file_name);//��������� obj-������
bool ProcessingOUTFiles(const std::string &path, const uint32_t &address, const uint32_t &length);//��������� out-������
bool isHex(const std::string &str); /*  �������� - �������� �� ������ ����������������� ������?  */


//----------------------------------------------------------------------------------------------------
//������� ������� ���������
//----------------------------------------------------------------------------------------------------
int WINAPI WinMain(HINSTANCE hInstance,HINSTANCE hPrevstance,LPSTR lpstrCmdLine,int nCmdShow)
{
 LPWSTR *argv;
 int argc;
 argv=CommandLineToArgvW(GetCommandLineW(),&argc);

 uint32_t address = 0x2000;
 uint32_t length  = 0x4000;

 printf("\r\nUsage: TMS320Make.exe --adress 0x2000 --length 0x4000\r\n");

 if (argc > 2) 
 {
   /* ������ ���� ��� ������� ��� argv[] (��� � hex-��������), ����
   --adsress 0x2000 �/���
   --length 0x4000
   ���� ���-�� �� ���, �� ������������ ��������� ��������.
   ����� ���� ������� ��� "0x"???
   */
   for (int i=1;i<argc-1;i++) 
   {
    char *p;
    if (!strcmp((char*)argv[i],(char*)L"--address") && (isHex((char*)argv[i + 1]))) 
	{
     address=strtol((char*)argv[i+1],&p,16);
    }
	if (!strcmp((char*)argv[i],(char*)L"--length") && (isHex((char*)argv[i + 1]))) 
	{
     length=strtol((char*)argv[i + 1],&p,16);
    }
   }
  }

	

    if (Processing(address, length) == false) {
        fprintf(stderr," \r\nStop compilation.\r\n \r\n");
        exit(EXIT_FAILURE);
    }
    exit(EXIT_SUCCESS);
}

/*
 * �������� - �������� �� ������ ����������������� ������?
*/
bool isHex(const std::string &str)
{
   if (str.empty() || ((!isdigit(str[0])) && (str[0] != '-') && (str[0] != '+'))) return false;
   char *p;
   strtol(str.c_str(), &p, 16);
   return (*p == 0);
}


//----------------------------------------------------------------------------------------------------
//��������� �� ����������
//----------------------------------------------------------------------------------------------------
bool Processing(const uint32_t &address, const uint32_t &length)
{
 //������ ������� �������
 char path[MAX_PATH];
 GetCurrentDirectory(MAX_PATH,path);

 //������� ����������� ����
 std::string output_file_name=path;
 output_file_name+="\\out.out";
 DeleteFile(output_file_name.c_str());
 std::string output_dat_file_name=path;
 output_dat_file_name+="\\out.dat";
 DeleteFile(output_dat_file_name.c_str());
 std::string output_dat_crc_file_name=path;
 output_dat_crc_file_name+="\\out-crc.dat";
 DeleteFile(output_dat_crc_file_name.c_str());
 
 //������ ������� ��������������
 char module_file_name[MAX_PATH];
 GetModuleFileName(GetModuleHandle(NULL),module_file_name,MAX_PATH);
 PathRemoveFileSpec(module_file_name);
 std::string tools_file_name=module_file_name;
 tools_file_name+="\\";

 std::string libs_file_name="\""+tools_file_name+"lib"+"\"";
 std::string include_file_name="\""+tools_file_name+"include"+"\"";
 std::string ac30_file_name="\""+tools_file_name+"bin\\ac30.exe"+"\"";
 std::string opt30_file_name="\""+tools_file_name+"bin\\opt30.exe"+"\"";
 std::string cg30_file_name="\""+tools_file_name+"bin\\cg30.exe"+"\"";
 std::string asm30_file_name="\""+tools_file_name+"bin\\asm30.exe"+"\"";
 std::string lnk30_file_name="\""+tools_file_name+"bin\\lnk30.exe"+"\"";

 if (ProcessingCFiles(path,".c",ac30_file_name,include_file_name)==false) return(false);
 if (ProcessingCFiles(path,".cc",ac30_file_name,include_file_name)==false) return(false);
 if (ProcessingIFFiles(path,opt30_file_name)==false) return(false);
 if (ProcessingOPTFiles(path,cg30_file_name)==false) return(false);
 if (ProcessingASMFiles(path,asm30_file_name)==false) return(false);
 if (ProcessingOBJFiles(path,lnk30_file_name,libs_file_name)==false) return(false);

 if (ProcessingOUTFiles(path, address, length)==false) return(false);
 return(true);
}

//----------------------------------------------------------------------------------------------------
//���������
//----------------------------------------------------------------------------------------------------
void ProcessingFile(const std::string &path,const std::string &mask,std::vector<std::string> &file_array)
{
 FindFile(path,mask,file_array);
 FindPath(path,mask,file_array);
}
//----------------------------------------------------------------------------------------------------
//��������� ������
//----------------------------------------------------------------------------------------------------
void FindFile(const std::string &path,const std::string &mask,std::vector<std::string> &file_array)
{
 //��������������� �����
 WIN32_FIND_DATA wfd;
 HANDLE handle=FindFirstFile(mask.c_str(),&wfd);
 if (handle==INVALID_HANDLE_VALUE) return;
 while(true)
 {
  if (wfd.cFileName[0]!='.' && !(wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))//���� ��� ����
  {
   //��������� ���� � ������
   std::string file_name=path;
   file_name+="\\";
   file_name+=wfd.cFileName;
   //������� ���������� �����
   std::string name_we=file_name;
   name_we.erase(name_we.find_last_of("."),std::string::npos);
   file_array.push_back(name_we);
  }
  if (FindNextFile(handle,&wfd)==FALSE) break;
 }
 FindClose(handle);
}
//----------------------------------------------------------------------------------------------------
//��������� ���������
//----------------------------------------------------------------------------------------------------
void FindPath(const std::string &path,const std::string &mask,std::vector<std::string> &file_array)
{
 char current_directory[MAX_PATH];
 if (GetCurrentDirectory(MAX_PATH,current_directory)==FALSE) return;
 if (SetCurrentDirectory(path.c_str())==FALSE) return;
 //������������ ��������
 WIN32_FIND_DATA wfd;
 HANDLE handle=FindFirstFile("*",&wfd);
 if (handle==INVALID_HANDLE_VALUE) 
 {
  SetCurrentDirectory(current_directory);
  return;
 }
 while(true)
 {
  if (wfd.cFileName[0]!='.' && (wfd.dwFileAttributes&FILE_ATTRIBUTE_DIRECTORY))//���� ��� ����������
  {   
   std::string new_path=path;
   new_path+="\\";
   new_path+=wfd.cFileName;
   SetCurrentDirectory(new_path.c_str());
   ProcessingFile(new_path.c_str(),mask,file_array);
   SetCurrentDirectory(path.c_str());
  }
  if (FindNextFile(handle,&wfd)==FALSE) break;
 }
 FindClose(handle);
 SetCurrentDirectory(current_directory);
}
//----------------------------------------------------------------------------------------------------
//������ �� ����������
//----------------------------------------------------------------------------------------------------
DWORD Execute(const char *name,const char *param,const char *directory,bool use_stdout_and_stderror)
{
 PROCESS_INFORMATION pi;
 STARTUPINFO si;

 HANDLE duplicate_hStdOutput=INVALID_HANDLE_VALUE;
 HANDLE duplicate_hStdError=INVALID_HANDLE_VALUE;

 ZeroMemory(&si,sizeof(STARTUPINFO));
 si.cb=sizeof(STARTUPINFO);
 si.dwFlags=STARTF_USESHOWWINDOW|STARTF_FORCEOFFFEEDBACK;
 si.wShowWindow=SW_HIDE;

 if (use_stdout_and_stderror==true)
 {
  HANDLE hStdOutput=GetStdHandle(STD_OUTPUT_HANDLE);
  HANDLE hStdError=GetStdHandle(STD_ERROR_HANDLE);

  DuplicateHandle(GetCurrentProcess(),hStdOutput,GetCurrentProcess(),&duplicate_hStdOutput,0,TRUE,DUPLICATE_SAME_ACCESS);
  DuplicateHandle(GetCurrentProcess(),hStdError,GetCurrentProcess(),&duplicate_hStdError,0,TRUE,DUPLICATE_SAME_ACCESS);
  
  si.dwFlags|=STARTF_USESTDHANDLES;
  si.hStdOutput=duplicate_hStdOutput;
  si.hStdInput=GetStdHandle(STD_INPUT_HANDLE);
  si.hStdError=duplicate_hStdError;
 }

 std::string file_name=name;
 file_name+=" ";
 file_name+=param;
  
 if (CreateProcess(NULL,const_cast<char *>(file_name.c_str()),NULL,NULL,TRUE,CREATE_NEW_CONSOLE,NULL,NULL,&si,&pi)==FALSE)
 {  
  if (duplicate_hStdOutput!=INVALID_HANDLE_VALUE) CloseHandle(duplicate_hStdOutput);
  if (duplicate_hStdError!=INVALID_HANDLE_VALUE) CloseHandle(duplicate_hStdError);  
  fprintf(stderr,"Can not execute %s\r\n",name);
  return(EXIT_FAILURE);
 }
 
 if (pi.hProcess!=INVALID_HANDLE_VALUE)
 {
  DWORD dwExitCode=STILL_ACTIVE;
  while (dwExitCode==STILL_ACTIVE)
  {
   WaitForSingleObject(pi.hProcess,1000);
   GetExitCodeProcess(pi.hProcess,&dwExitCode);
  }
  CloseHandle(pi.hProcess);
  if (duplicate_hStdOutput!=INVALID_HANDLE_VALUE) CloseHandle(duplicate_hStdOutput);
  if (duplicate_hStdError!=INVALID_HANDLE_VALUE) CloseHandle(duplicate_hStdError);  
  return(dwExitCode);
 }
 
 if (duplicate_hStdOutput!=INVALID_HANDLE_VALUE) CloseHandle(duplicate_hStdOutput);
 if (duplicate_hStdError!=INVALID_HANDLE_VALUE) CloseHandle(duplicate_hStdError); 
 fprintf(stderr,"Can not execute %s\r\n",name);
 return(EXIT_FAILURE);
}


//----------------------------------------------------------------------------------------------------
//��������� c-������
//----------------------------------------------------------------------------------------------------
bool ProcessingCFiles(const std::string &path,const std::string &mask,const std::string &ac30_file_name,const std::string &include_file_name)
{
 printf("Compilation %s-file.\r\n",mask.c_str());

 std::vector<std::string> file_array;
 //����������� c-�����
 ProcessingFile(path,"*"+mask,file_array); 
 bool no_error=true;
 auto output_c_func=[&](const std::string &str)
 {
  std::string file_name="\""+str+mask+"\"";
  fprintf(stdout,"****************************************************************************************************\r\n");
  fprintf(stdout,"compilation %s\r\n",file_name.c_str());
  fprintf(stdout,"****************************************************************************************************\r\n");
  std::string param="-i"+include_file_name+" -k -v30 -als -fr ";
  param+=file_name;
  if (Execute(ac30_file_name.c_str(),param.c_str(),str.c_str(),false)!=EXIT_SUCCESS) no_error=false;
  
  //������� err-����, ���� �� ����
  std::string err_file_name=str+".err";
  FILE *file=fopen(err_file_name.c_str(),"rb");
  if (file!=NULL)
  {
   fprintf(stdout,"ERROR!\r\n \r\n");
   no_error=false;   
   while(true)
   {
    char b;
	if (fread(&b,sizeof(char),1,file)==0) break;
	fprintf(stdout,"%c",b);
   }
   fclose(file);
  }
  else fprintf(stdout,"ok.\r\n");
  fprintf(stdout," \r\n");
 }; 
 std::for_each(file_array.begin(),file_array.end(),output_c_func);
 return(no_error);
}
//----------------------------------------------------------------------------------------------------
//��������� if-������
//----------------------------------------------------------------------------------------------------
bool ProcessingIFFiles(const std::string &path,const std::string &opt30_file_name)
{
 printf("Optimize if-file.\r\n");

 std::vector<std::string> file_array;
 ProcessingFile(path,"*.if",file_array);
 bool no_error=true;

 auto output_if_func=[&](const std::string &str)
 {
  std::string file_name="\""+str+".if\"";
  printf("optimize %s\r\n",file_name.c_str());
  if (Execute(opt30_file_name.c_str(),file_name.c_str(),str.c_str(),false)!=EXIT_SUCCESS) no_error=false;
 }; 
 std::for_each(file_array.begin(),file_array.end(),output_if_func);
 return(no_error);
}
//----------------------------------------------------------------------------------------------------
//��������� opt-������
//----------------------------------------------------------------------------------------------------
bool ProcessingOPTFiles(const std::string &path,const std::string &cg30_file_name)
{
 printf("Create asm-file.\r\n");

 std::vector<std::string> file_array;
 ProcessingFile(path,"*.opt",file_array);
 bool no_error=true;

 auto output_opt_func=[&](const std::string &str)
 {
  std::string file_name="\""+str+".opt\"";
  printf("create asm %s\r\n",file_name.c_str());
  if (Execute(cg30_file_name.c_str(),file_name.c_str(),str.c_str(),false)!=EXIT_SUCCESS) no_error=false;
 }; 
 std::for_each(file_array.begin(),file_array.end(),output_opt_func);
 return(no_error);
}
//----------------------------------------------------------------------------------------------------
//��������� asm-������
//----------------------------------------------------------------------------------------------------
bool ProcessingASMFiles(const std::string &path,const std::string &asm30_file_name)
{
 printf("Compile asm-file.\r\n");
 std::vector<std::string> file_array;
 ProcessingFile(path,"*.asm",file_array);
 bool no_error=true;

 auto output_asm_func=[&](const std::string &str)
 {
  std::string file_name="\""+str+".asm\"";
  printf("create obj %s\r\n",file_name.c_str());
  std::string param="-ls -v30 ";
  param+=file_name;
  if (Execute(asm30_file_name.c_str(),param.c_str(),str.c_str(),false)!=EXIT_SUCCESS) no_error=false;
 }; 
 std::for_each(file_array.begin(),file_array.end(),output_asm_func);
 return(no_error);
}
//----------------------------------------------------------------------------------------------------
//��������� obj-������
//----------------------------------------------------------------------------------------------------
bool ProcessingOBJFiles(const std::string &path,const std::string &lnk30_file_name,const std::string &libs_file_name)
{
 printf("Create out-file.\r\n");

 std::vector<std::string> file_array;
 //�������� ������
 ProcessingFile(path,"*.obj",file_array);
 std::string obj_list;
 auto output_obj_func=[&](const std::string &str)
 {
  std::string file_name="\""+str+".obj\"";
  obj_list+=file_name+" ";
 }; 
 std::for_each(file_array.begin(),file_array.end(),output_obj_func);

 std::string param="-c -m out.map -o out.out -x -i ";
 param+=libs_file_name;
 param+=" -l rts30.lib ";
 param+=obj_list;
 param+=" c30.cmd";

 if (Execute(lnk30_file_name.c_str(),param.c_str(),path.c_str(),true)!=EXIT_SUCCESS) return(false);
 return(true);
}

//----------------------------------------------------------------------------------------------------
//��������� out-������
//----------------------------------------------------------------------------------------------------
bool ProcessingOUTFiles(const std::string &path, const uint32_t &address, const uint32_t &length)
{
 printf("Create dat-file.\r\n");
 std::vector<uint8_t> out_file_data;
 FILE *file_out=fopen((path+"\\out.out").c_str(),"rb");
 if (file_out==NULL)
 {
  printf("Error open out.out!\r\n");
  return(false);
 }
 while(1)
 {
  uint8_t s;
  if (fread(&s,sizeof(uint8_t),1,file_out)==0) break;
  out_file_data.push_back(s);
 }
 fclose(file_out);

 //���� ������ ������ � ������ ����������
 std::vector<uint8_t> data_section_name;
 data_section_name.push_back('.');
 data_section_name.push_back('d');
 data_section_name.push_back('a');
 data_section_name.push_back('t');
 data_section_name.push_back('a');

 std::vector<uint8_t> boot_section_name;
 boot_section_name.push_back('b');
 boot_section_name.push_back('o');
 boot_section_name.push_back('o');
 boot_section_name.push_back('t');
 boot_section_name.push_back('.');
 boot_section_name.push_back('a');
 boot_section_name.push_back('s');
 boot_section_name.push_back('m');

 std::vector<uint8_t>::iterator iterator_data_section=std::search(out_file_data.begin(),out_file_data.end(),data_section_name.begin(),data_section_name.end());
 if (iterator_data_section==out_file_data.end())
 {
  printf("Error find .data section in out.out file!\r\n");
  return(false);
 }
 std::vector<uint8_t>::iterator iterator_boot_section=std::search(out_file_data.begin(),out_file_data.end(),boot_section_name.begin(),boot_section_name.end());
 if (iterator_boot_section==out_file_data.end())
 {
  printf("Error find boot.asm section in out.out file!\r\n");
  return(false);
 }

 std::vector<uint8_t> dat_file_data;

 size_t size_dat_file=0;
 iterator_data_section+=48;//������������ � ������� ����� ������ 
 size_t counter=11*4;//������-�� ����� ������ ���������
 while(iterator_data_section<iterator_boot_section && counter>0)
 {
  uint8_t b;
  b=*iterator_data_section;
  iterator_data_section++;
  counter--;
  size_dat_file+=sizeof(uint8_t);
  dat_file_data.push_back(b);
 }
 //���������� ������ (������ ����������?)
 uint32_t l1=0xFB000002;
 uint32_t l2=0xFB000100;
 for(size_t n=0;n<26;n++)
 {
  size_dat_file+=sizeof(uint32_t);
  dat_file_data.push_back((l1>>0)&0xff);
  dat_file_data.push_back((l1>>8)&0xff);
  dat_file_data.push_back((l1>>16)&0xff);
  dat_file_data.push_back((l1>>24)&0xff);

  size_dat_file+=sizeof(uint32_t);
  dat_file_data.push_back((l2>>0)&0xff);
  dat_file_data.push_back((l2>>8)&0xff);
  dat_file_data.push_back((l2>>16)&0xff);
  dat_file_data.push_back((l2>>24)&0xff);
 }
 size_dat_file+=sizeof(uint32_t);
 dat_file_data.push_back((l1>>0)&0xff);
 dat_file_data.push_back((l1>>8)&0xff);
 dat_file_data.push_back((l1>>16)&0xff);
 dat_file_data.push_back((l1>>24)&0xff);

 //������
 while(iterator_data_section<iterator_boot_section)
 {
  uint8_t b;
  b=*iterator_data_section;
  iterator_data_section++;
  size_dat_file+=sizeof(uint8_t);
  dat_file_data.push_back(b);
 }
 //��������� ���� � ����� �����
 SYSTEMTIME systemtime;
 GetLocalTime(&systemtime);
 static const size_t STRING_BUFFER_SIZE=255;
 char date[STRING_BUFFER_SIZE];
 char time[STRING_BUFFER_SIZE];
 sprintf(date,"%02i%02i%04i",systemtime.wDay,systemtime.wMonth,systemtime.wYear);
 sprintf(time,"%02iE%02iE%02i",systemtime.wHour,systemtime.wMinute,systemtime.wSecond);

 auto string_convert=[](const char *string)->uint32_t
 {
  size_t length=strlen(string);
  uint32_t value=0;
  for(size_t n=0;n<length;n++)
  {
   value<<=4;
   unsigned char s=string[n];
   if (s>='0' && s<='9') value|=s-'0';
   if (s>='A' && s<='F') value|=s-'A'+0x0A;
   if (s>='a' && s<='f') value|=s-'a'+0x0A;
  }
  return(value);
 };

 uint32_t date_value=string_convert(date);
 uint32_t time_value=string_convert(time);


 printf("________$________\r\n");
 printf("_______$_$_______\r\n");
 printf("______$___$______\r\n");
 printf("_____$_____$_____\r\n");
 printf("____$_______$____\r\n");
 printf("____$_______$____\r\n");
 printf("____$_______$____\r\n");
 printf("____$_______$____\r\n");
 printf("____$_______$____\r\n");
 printf("____$_______$____\r\n");
 printf("____$_______$____\r\n");
 printf("___$__$___$__$___\r\n");
 printf("__$__$_$_$_$__$__\r\n");
 printf("_$__$__$$$__$__$_\r\n");
 printf("_$$$____$____$$$_\r\n");
 printf("_$______$______$_\r\n");
 printf("$_______$_______$\r\n");

 printf("Date:%s (0x%08x)\r\n",date,date_value);
 printf("Time:%s (0x%08x)\r\n",time,time_value);
 
 //������������ ����
 static const size_t END_WORD_AMOUNT=4;//���������� ���� � �����
 while(size_dat_file<65536-sizeof(uint32_t)*END_WORD_AMOUNT)
 {
  uint8_t b=0xff;
  size_dat_file+=sizeof(uint8_t);
  dat_file_data.push_back(b);
 }
 //��������� ����
 dat_file_data.push_back((date_value>>0)&0xff);
 dat_file_data.push_back((date_value>>8)&0xff);
 dat_file_data.push_back((date_value>>16)&0xff);
 dat_file_data.push_back((date_value>>24)&0xff);
 
 //��������� �����
 dat_file_data.push_back((time_value>>0)&0xff);
 dat_file_data.push_back((time_value>>8)&0xff);
 dat_file_data.push_back((time_value>>16)&0xff);
 dat_file_data.push_back((time_value>>24)&0xff);

 //��������� ������ � ������
 dat_file_data.push_back(0xff);
 dat_file_data.push_back(0xff);
 dat_file_data.push_back(0xff);
 dat_file_data.push_back(0xff);
 
 //������� ����������� ����� � ������ ���� � ���
 //������ dat-����
 FILE *file_dat=fopen((path+"\\out.dat").c_str(),"wb");
 if (file_dat==NULL)
 {
  printf("Error create out.dat!\r\n");
  return(false);
 }

 FILE *file_crc_dat=fopen((path+"\\out-crc.dat").c_str(),"wb");
 if (file_crc_dat==NULL)
 {
  printf("Error create out-crc.dat!\r\n");
  fclose(file_dat);
  return(false);
 }
 
 fprintf(file_crc_dat,"1651 1 %x 0 %x\r\n", address, length);
 fprintf(file_dat,"1651 1 0 0 3fff\r\n");
 uint32_t crc=0;
 size_t size=dat_file_data.size();
 for(size_t n=0;n<size;)
 {
  uint32_t b0=dat_file_data[n++];
  uint32_t b1=dat_file_data[n++];
  uint32_t b2=dat_file_data[n++];
  uint32_t b3=dat_file_data[n++];

  uint32_t word=0;
  word<<=8;word+=b3;
  word<<=8;word+=b2;
  word<<=8;word+=b1;
  word<<=8;word+=b0;

  crc+=word;


  fprintf(file_crc_dat,"0x%08x\r\n",word);
  fprintf(file_dat,"0x%08x\r\n",word);
 }
 fprintf(file_crc_dat,"0x%08x\r\n",crc);
 fclose(file_dat);
 fclose(file_crc_dat); 
 printf("CRC: 0x%08x\r\n",crc);
 return(true);
}
