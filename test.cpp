#include <iostream>
#include "libalphatree/alphaforest.h"
#include "libalphatree/alpha/alpha.h"
#include "libalphatree/alpha/base.h"
#include "libalphatree/base/threadpool.h"
#include "libalphatree/alpha/normal.h"
#include <thread>

using namespace std;

class ThreadPoolTest{
    public:
        int consumer(){
            std::unique_lock<std::mutex> lock{mutex_};
            //如果某个缓存被用完,需要等待别人归还后再去使用,如果条件不满足,会mutex_.unlock()然后等待
            //当别人notify_one时会再去检查条件,并mutex_.lock()
            cout<<"start consumer!"<<endl;
            cv_.wait(lock, [this]{ return num > 0;});
            cout<<"finish consumer! num="<<num<<endl;
            num--;
            return num;
        }

        int producer(){
            {
                std::unique_lock<std::mutex> lock{mutex_};
                num++;
                cout<<"finish producer! num="<<num<<endl;
            }
            cv_.notify_one();
            return num;
        }
    private:
        int num{0};
        //线程安全
        std::mutex mutex_;
        std::condition_variable cv_;
};

class CExample {
private:
    int a;
public:
    //构造函数
    CExample(int b)
    { a = b;cout<<"000"<<endl;
    }

    ~CExample(){cout<<"dddd"<<a<<endl;}

    //拷贝构造函数
    CExample(const CExample& C)
    {
        a = C.a;
        cout<<"aaa"<<a<<endl;
    }

    //一般函数
    void Show ()
    {
        cout<<a<<endl;
    }
};

CExample f1(){
    cout<<"f1"<<endl;
    CExample a = CExample(1);
    CExample d = CExample(2);
    return a;
}

CExample f2(){
    cout<<"f2"<<endl;
    return CExample(3);
}

int countdown (int from, int to) {
    for (int i=from; i!=to; --i) {
        std::cout << i << '\n';
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }
    std::cout << "Finished!\n";
    return from - to;
}

int main() {

    cout<<"test thread ------------------"<<endl;
    std::thread* th;
    std::future<int> ff[10];
    {
        std::packaged_task<int(int,int)> task(countdown); // 设置 packaged_task
        ff[0] = task.get_future(); // 获得与 packaged_task 共享状态相关联的 future 对象.

        th = new std::thread(std::move(task), 10, 0);   //创建一个新线程完成计数任务.
    }

    cout<<"start"<<endl;

    int value = ff[0].get();                    // 等待任务完成并获取结果.

    std::cout << "The countdown lasted for " << value << " seconds.\n";

    th->join();
    delete th;

    cout<<"test---------"<<endl;
    cout<<normSDist(2.0)<<" "<<normsinv(0.9772)<<endl;

    CExample b = f1();
    cout<<"-----"<<endl;
    std::vector<CExample> v;
    {
        CExample dd = f2();
        v.push_back(std::move(dd));
    }
    v[0].Show();
    cout<<"-----"<<endl;
    CExample c(4);
    CExample d = std::move(c);

    ThreadPool threadPool(2);
    ThreadPoolTest* tt = new ThreadPoolTest();
    {
        auto t1 = threadPool.enqueue([tt]{ return tt->consumer();});
        auto t2 = threadPool.enqueue([tt]{ return tt->producer();});
        cout<<"!!!----"<<endl;
        cout<<t1.get()<<" "<<t2.get()<<" "<<t1.get()<<endl;
        cout<<"!!!-----------"<<endl;
    }




    AlphaForest::initialize(251, 2048, 10, 5, 4, 20, 40, 10, 4096, 20);
    char str[1024];
    char outstr[1024];

    int id = AlphaForest::getAlphaforest()->useAlphaTree("(indneutralize((correlation(delta(close, 1), delta(delay(close, 1), 1), 250) * (delta(close, 1) / close)), IndClass.market) / sum(((delta(close, 1) / delay(close, 1)) ^ 2), 250))");
    cout<<AlphaForest::getAlphaforest()->encodeAlphaTree(id, str)<<endl;
    int ids[] = {id};
    int substr = AlphaForest::getAlphaforest()->summarySubAlphaTree(ids,1,2,outstr);
    char* p = outstr;
    while(substr--){
        cout<<p<<endl;
        p += (strlen(p)+1);
    }

    return 0;
}