#include <iostream>
#include <mutex>
#include <queue>
#include <sstream>
#include <thread>
using namespace std;

// COLA CONCURRENTE 

template <class T>
struct CNode
{
    T value; // valor del nodo
    CNode<T>* next;
    atomic<bool> marked = false; // variable atomica que indica si un nodo esta siendo eliminado
    recursive_mutex mut; // recursive mutex para hacer lock y evitar problemas de interlock
    void lock()
    {
        mut.lock();
    }
    void unlock()
    {
        mut.unlock();
    }
    CNode(T x)
    {
        value = x;
        next = nullptr;
    }
};

template <class T>
struct ConcurrentQueue
{
    CNode<T>* head, * tail;
    ConcurrentQueue();
    ~ConcurrentQueue();
    bool push(const T& data); // Funcion insertar, inserta desde la cola
    void search(CNode<T>*& pred, CNode<T>*& succ); // Funcion para apuntar al ultimo nodo insertado
    T pop(); // Funcion eliminar, elimina desde la cabeza
};

template <class T>
ConcurrentQueue<T>::~ConcurrentQueue()
{
    CNode<T>* p = head;
    while (p)
    {
        pop();
    }
}

template <class T>
ConcurrentQueue<T>::ConcurrentQueue()
{
    head = new CNode<T>(T()); // Inicializo head con el constructor del tipo de dato
    tail = new CNode<T>(T()); // Inicializo tail con el constructor del tipo de dato
    head->next = tail; // Uno el nodo head al nodo tail
}

template <class T>
void ConcurrentQueue<T>::search(CNode<T>*& pred, CNode<T>*& succ)
{
    pred = head; // pred y succ apuntan a la cabeza y al nodo siguiente respectivamente
    succ = head->next;
    if (succ->next)
    {
        while (succ != tail) // Avanzo hasta llegar al ultimo nodo insertado
        {
            pred = succ;
            succ = pred->next;
        }
    }
}


template <class T>
bool ConcurrentQueue<T>::push(const T& data)
{
    CNode<T>* pred = nullptr;
    CNode<T>* succ = nullptr;
    bool res = false; // indica si se pudo realizar la inserción o no

    while (1)
    {
        search(pred, succ); // se ubica a pred y succ al final de la cola 
        pred->lock(); // se bloquea al predecesor y sucesor
        succ->lock();
        if (!(pred->marked) && !(succ->marked) && pred->next == succ) // se comprueba que el predecesor y sucesor no estén siendo eliminados y que se mantengan conectados
        {

            CNode<T>* nuevoNodo = new CNode<T>(data);
            nuevoNodo->next = succ;
            pred->next = nuevoNodo;
            res = true;
            // luego de la inserción se desbloquean los nodos pred y succ y se retorna res.
            pred->unlock();
            succ->unlock();
            return res;
        }// si la inserción no fue válida, se desbloquean los nodos que fueron bloqueados al inicio
        pred->unlock();
        succ->unlock();
    }
}

template <class T>
T ConcurrentQueue<T>::pop()
{
    CNode<T>* pred = nullptr;
    CNode<T>* succ = nullptr;
    bool res = false; // indica si se pudo realizar la inserción o no

    while (1)
    {
        pred = head; // pred y succ apuntan a la cabeza y al nodo siguiente respectivamente
        succ = head->next;

        pred->lock(); // se bloquea al predecesor y sucesor
        succ->lock();
        if (!(pred->marked) && !(succ->marked) && pred->next == succ) // se comprueba que el predecesor y sucesor no estén siendo eliminados y que se mantengan conectados
        {
            if (succ->value == tail->value)
            {
                res = false;
            }
            else
            {
                succ->marked = true;
                pred->next = succ->next;
                res = true;
            }

            // luego del borrado se desbloquean los nodos pred y succ y se retorna el valor del nodo eliminado.
            pred->unlock();
            succ->unlock();
            return succ->value;
        }// si el borrado no fue válido, se desbloquean los nodos que fueron bloqueados al inicio
        pred->unlock();
        succ->unlock();
    }
}



class Producer {
public:
    Producer(unsigned int id, ConcurrentQueue<std::string>* queue)
        : id_(id), queue_(queue) {}

    void operator()() {
        int data = 0;
        while (true) {
            std::stringstream stream;
            stream << "Producer: " << id_ << " Data: " << data++ << std::endl;
            queue_->push(stream.str());
            std::cout << stream.str() << std::endl;
        }
    }

private:
    unsigned int id_;
    ConcurrentQueue<std::string>* queue_;
};

class Consumer {
public:
    Consumer(unsigned int id, ConcurrentQueue<std::string>* queue)
        : id_(id), queue_(queue) {}

    void operator()() {
        while (true) {
            std::stringstream stream;
            stream << "Consumer: " << id_ << " Data: " << queue_->pop().c_str()
                << std::endl;

            std::cout << stream.str() << std::endl;
        }
    }

private:
    unsigned int id_;
    ConcurrentQueue<std::string>* queue_;
};

int main(int argc, char* argv[]) {
    if (argc != 3) {
        return 1;
    }
    int number_producers = std::stoi(argv[1]);
    int number_consumers = std::stoi(argv[2]);

    ConcurrentQueue<std::string> queue;

    std::vector<std::thread*> producers;
    for (unsigned int i = 0; i < number_producers; ++i) {
        std::thread* producer_thread = new std::thread(Producer(i, &queue));
        producers.push_back(producer_thread);
    }

    std::vector<std::thread*> consumers;
    for (unsigned int i = 0; i < number_consumers; ++i) {
        std::thread* consumer_thread = new std::thread(Consumer(i, &queue));
        consumers.push_back(consumer_thread);
    }

    int stop;
    std::cin >> stop;
    // join

    return 0;
}