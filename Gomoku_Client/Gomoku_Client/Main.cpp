#include <thread>
#include "Client.h"
#include "Gui.h"

int main()
{ 
    //Client)  
    boost::asio::io_context io;
    Client client(io);
      

    //Thread)
    std::thread clientThread;
    try{clientThread = std::thread([&io]() {io.run(); });}
    catch (const std::exception& e)
    {
        //std::cout << "Client Error : " << e.what() << std::endl;
    }

    //imgui
    Gui gui(client);

    client.Set_Callback([&gui](const std::string& str) {gui.Dispatch_Data(str); });

    gui.Run();

    //Cleanup
    clientThread.join();    

    

    return 0;
}