protoc .\addressbook.proto --cpp_out=.\  
xcopy .\addressbook.pb.h ..\include\ /y
pause