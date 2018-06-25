# 引用asio库配置属性
## 常规
输出目录：$(SolutionDir)bin
中间目录：$(SolutionDir)build\$(ProjectName)
## C/C++
常规->附加包含目录：
```
$(SolutionDir)
$(SolutionDir)asio\third\boost\include
$(SolutionDir)asio\third\protobuf\include
$(SolutionDir)asio\third\glog\include
```
## 链接器
常规->附加库目录：
```
$(SolutionDir)asio\third\boost\lib
$(SolutionDir)asio\third\protobuf\lib
$(SolutionDir)asio\third\glog\lib
```
输入->附加依赖项：
```
libboost_system-vc120-mt-gd-1_65_1.lib
libprotobufd.lib
glogd.lib
```