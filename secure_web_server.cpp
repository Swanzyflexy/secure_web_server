#include <cstdlib>
#include <iostream>
#include <fstream>
#include <sstream>
#include <thread>
#include <utility>
#include <boost/asio.hpp>
#include <boost/asio/ts/buffer.hpp>
#include <boost/asio/ts/internet.hpp>

using boost::asio::ip::tcp;

const int max_length = 1024;

std::string get_response(const std::string& request)
{
    std::string response;

    std::istringstream iss(request);
    std::string method, path, protocol;
    iss >> method >> path >> protocol;

    if (method == "GET")
    {
        std::string filename = "." + path;

        std::ifstream file(filename, std::ios::binary);
        if (file)
        {
            std::ostringstream oss;
            oss << file.rdbuf();
            file.close();

            response += "HTTP/1.1 200 OK\r\n";
            response += "Content-Type: text/html\r\n";
            response += "Content-Length: " + std::to_string(oss.str().length()) + "\r\n";
            response += "\r\n";
            response += oss.str();
        }
        else
        {
            response += "HTTP/1.1 404 Not Found\r\n";
            response += "\r\n";
            response += "<h1>404 Not Found</h1>";
        }
    }
    else if (method == "POST")
    {
        std::string line;
        bool isFormSubmission = false;
        std::string formSubmissionData;

        // Read the request headers
        while (std::getline(iss, line) && line != "\r")
        {
            // Check if it's a form submission
            if (line.find("Content-Type: application/x-www-form-urlencoded") != std::string::npos)
            {
                isFormSubmission = true;
            }
        }

        if (isFormSubmission)
        {
            // Read the form submission data
            std::getline(iss, formSubmissionData, '\0');

            // Save/store the form submission data
            std::ofstream outfile("submissions.txt", std::ios::app);
            if (outfile)
            {
                outfile << formSubmissionData << std::endl;
                outfile.close();
            }
        }

        response += "HTTP/1.1 200 OK\r\n";
        response += "Content-Type: text/html\r\n";
        response += "\r\n";
        response += "<h1>Form Submitted Successfully!</h1>";
    }
    else
    {
        response += "HTTP/1.1 400 Bad Request\r\n";
        response += "\r\n";
        response += "<h1>400 Bad Request</h1>";
    }

    return response;
}

void session(std::shared_ptr<tcp::socket> sock)
{
    try
    {
        char data[max_length];
        boost::system::error_code error;

        size_t length = sock->read_some(boost::asio::buffer(data), error);
        if (!error)
        {
            std::string request(data, data + length);
            std::string response = get_response(request);

            boost::asio::write(*sock, boost::asio::buffer(response));
        }
        else if (error != boost::asio::error::eof)
        {
            throw boost::system::system_error(error);
        }
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception in thread: " << e.what() << "\n";
    }
}



int main(int argc, char* argv[])
{
    try
    {
        if (argc != 2)
        {
            std::cerr << "Usage: secure_web_server <port>\n";
            return 1;
        }

        boost::asio::io_context io_context;

        unsigned short port = std::atoi(argv[1]);
        server(io_context, port);

        io_context.run();
    }
    catch (std::exception& e)
    {
        std::cerr << "Exception: " << e.what() << "\n";
    }

    return 0;
}