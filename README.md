json-rpc
========

json-rpc is an implementation of rpc laying on top of json-formatted message communication.


## Build
json-rpc depends on my several repositories. Before you install json-rpc, you have to download and install them.

  * [common](https://github.com/allenbo/common.git)
  * [JConer](https://github.com/allenbo/jconer.git)

Please follow the steps to install those packages
```
# clone common and jconer repos
git clone https://github.com/allenbo/common.git
git clone https://github.com/allenbo/JConer.git
# compile jconer library
cd JConer && cp -R ../common/include/common include && make
# compile json-rpc library
cd ../
git clone https://github.com/allenbo/json-rpc.git && cd json-rpc && mkdir lib
cd -R ../JConer/include/jconer include/ && cp ../JConer/libjconer.a lib/
cp -R ../common/include/common include/
make
```

## Setup
Unlike other rpc implementation, json-rpc needs you to write a json-formatted file desrciping your client and service.
Here is a demo example.

```
{
  "namespace" : "Demo",

  "classes" : {
    "Point" : {
      "x" : "int",
      "y" : "string"
    },
    "Rectangle" : {
      "from" : "Point", 
      "to" : "Point"
    }
  },

  "service" : [
    {
      "name" : "sayHello",
      "params" : {
        "name" : "string", 
        "age" : "int"
      }
    }, 
    {
      "name" : "getRandomNumber",
      "params" : null,
      "return" : "int"
    }
  ]
}
```

In this file, you define two serializable classes `Point` and `Rectangle` and a bunch of service function.
And of course, you specify your service name by set namespace to "Demo". So it will generate a header file
called `DemoCppStub.hpp`. In this file, a client class `DemoClient` and a service class `DemoService` will be
generated. And the generated file requires you to fill up the definition of the service functions you just
defined.

```
    virtual void sayHello(int age, string name){
      //TODO: stub HERE
    }
    virtual int getRandomNumber(){
      //TODO: stub HERE
    }
```

This is how you generate the stub file
```
bin/stubgen spec.json
```

## Demo
After you add function definitions, you have to compile your client and service binary seperately.
Here is an easy-to-go example showing how to do this.

```
DemoClient.cpp

#include <iostream>
#include "DemoCppStub.hpp"
#include "json-rpc/all.hpp"


int main() {
  SockClient sclient("127.0.0.1", "8199");
  DemoClient demo(sclient);
  demo.sayHello(21, "Justin");
  int number = demo.getRandomNumber();
  std::cout << "The number is " << number << std::endl;
}
```

```
DemoService.cpp

#include "DemoCppStub.hpp"
#include "json-rpc/all.hpp"

int main() {
  SockServer sserver("8199");
  DemoService demo(sserver);
  
  demo.listen();
  while(true) {}
}
```

This is how you compile these two files
```
g++ -o DemoClient DemoClient.cpp -I. -I./include -L. -L./lib -ljson-rpc -ljconer-lpthread -std=c++11
g++ -o DemoService DemoService.cpp -I. -I./include -L. -L./lib -ljson-rpc -jconer -lpthread -std=c++11
```
