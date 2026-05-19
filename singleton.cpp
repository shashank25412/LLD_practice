#include <iostream>
#include <mutex>
using namespace std;

class Singleton{
 private:
    Singleton(){
        cout<<"Singleton's constructor is getting initialized"<<endl;
    }

    static Singleton* aObject;
    static mutex mtx;

 public:
    static Singleton* getInstance(){
        if(aObject == nullptr){
            lock_guard<mutex> lock(mtx); // lock for thread safety

            if(aObject == nullptr){
                aObject = new Singleton();
            }
        }

        return aObject;
    }
};

Singleton* Singleton::aObject;
mutex Singleton::mtx;

int main(){

    Singleton* s1 = Singleton::getInstance();
    Singleton* s2 = Singleton::getInstance();

    cout<<(s1==s2)<<endl;

    return 0;
}