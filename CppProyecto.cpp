#include <iostream>
#include <string>
#include <memory>
#include <chrono>
#include <thread>
#include <windows.h> 
using namespace std;


class ModuloLectorSerial {
private:
    string puertoCom;
    unsigned long baudRate;
    bool conectado;
    string bufferInterno;
    HANDLE hSerial; 

public:
    ModuloLectorSerial(const string& puerto, unsigned long baudios = 115200)
        : puertoCom(puerto), baudRate(baudios), conectado(false), hSerial(INVALID_HANDLE_VALUE) {}

    ~ModuloLectorSerial() {
        desconectar();
    }

    bool conectar() {
        
        string puertoFormateado = "\\\\.\\" + puertoCom;
        
        hSerial = CreateFileA(puertoFormateado.c_str(), GENERIC_READ | GENERIC_WRITE, 0, NULL, OPEN_EXISTING, 0, NULL);
        if (hSerial == INVALID_HANDLE_VALUE) return false;

        // Configuración de los parámetros del puerto COM
        DCB dcbParams = { 0 };
        dcbParams.DCBlength = sizeof(dcbParams);
        if (!GetCommState(hSerial, &dcbParams)) return false;

        dcbParams.BaudRate = baudRate;
        dcbParams.ByteSize = 8;
        dcbParams.StopBits = ONESTOPBIT;
        dcbParams.Parity = NOPARITY;
        if (!SetCommState(hSerial, &dcbParams)) return false;

        
        COMMTIMEOUTS timeouts = { 0 };
        timeouts.ReadIntervalTimeout = 10;
        timeouts.ReadTotalTimeoutConstant = 10;
        timeouts.ReadTotalTimeoutMultiplier = 1;
        SetCommTimeouts(hSerial, &timeouts);
        
        conectado = true;
        return true;
    }

    void desconectar() {
        if (conectado && hSerial != INVALID_HANDLE_VALUE) {
            CloseHandle(hSerial);
            hSerial = INVALID_HANDLE_VALUE;
            conectado = false;
        }
    }

    
    string leerBuffer() {
        if (!conectado) return "";
        
        char tempBuffer[128];
        DWORD bytesLeidos = 0; 

        ReadFile(hSerial, tempBuffer, sizeof(tempBuffer) - 1, &bytesLeidos, NULL);

        if (bytesLeidos > 0) {
            tempBuffer[bytesLeidos] = '\0';
            bufferInterno += string(tempBuffer);
        }
        return bufferInterno;
    }

    
    string recibirTrama() {
        leerBuffer();
        size_t pos = bufferInterno.find('\n');
        if (pos != string::npos) {
            string trama = bufferInterno.substr(0, pos);
            if (!trama.empty() && trama.back() == '\r') {
                trama.pop_back(); 
            }
            bufferInterno.erase(0, pos + 1);
            return trama;
        }
        return ""; 
    }

    bool estaConectado() const { return conectado; }
};


class ContadoresEstado {
private:
    int totalRojas = 0;
    int totalAzules = 0;
    int totalAmarillas = 0;

public:
    void incrementar(const string& color) {
        if (color == "ROJO") totalRojas++;
        else if (color == "AZUL") totalAzules++;
        else if (color == "AMARILLO") totalAmarillas++;
    }

    int getTotalRojas() const { return totalRojas; }
    int getTotalAzules() const { return totalAzules; }
    int getTotalAmarillas() const { return totalAmarillas; }

    int obtenerTotal() const {
        return totalRojas + totalAzules + totalAmarillas;
    }
};


class InterfazUsuario {
public:
    void mostrarEstado(const string& estado) {
        cout << "[SISTEMA BANDA] Estado actual: " << estado << endl;
    }

    void mostrarContadores(const ContadoresEstado& contadores) {
       
        cout << " Cajas Rojas     : " << contadores.getTotalRojas() << endl;
        cout << " Cajas Azules    : " << contadores.getTotalAzules() << endl;
        cout << " Cajas Amarillas : " << contadores.getTotalAmarillas() << endl;
        cout << "----------------------------------------" << endl;
        cout << " Total Procesado : " << contadores.obtenerTotal() << endl;
        
    }
};


class LogicaClasificacion {
private:
    shared_ptr<ContadoresEstado> contadores;
    shared_ptr<InterfazUsuario> ui;

public:
    LogicaClasificacion(shared_ptr<ContadoresEstado> c, shared_ptr<InterfazUsuario> u)
        : contadores(c), ui(u) {}

    void interpretarTrama(const string& trama) {
        if (trama.empty()) return;

        if (trama.rfind("ESTADO:", 0) == 0) {
            string estado = trama.substr(7);
            ui->mostrarEstado(estado);
        }
        else if (trama.rfind("COLOR:", 0) == 0) {
            string color = trama.substr(6);
            clasificarProducto(color);
        }
    }

    void clasificarProducto(const string& color) {
        contadores->incrementar(color);
        ui->mostrarContadores(*contadores);
    }
};


int main() {
    string puerto;
    cout << "Ingrese el puerto COM de Arduino (Ej: COM3): ";
    cin >> puerto;

    
    ModuloLectorSerial lectorSerial(puerto, 115200);

    if (!lectorSerial.conectar()) {
        cerr << "Error: No se pudo abrir el puerto " << puerto << ". Verifique que el Arduino este conectado y el puerto no este en uso." << endl;
        return 1;
    }

    cout << "Conexion serial establecida correctamente en " << puerto << endl;
   

    auto contadores = make_shared<ContadoresEstado>();
    auto ui = make_shared<InterfazUsuario>();
    LogicaClasificacion clasificador(contadores, ui);


    while (lectorSerial.estaConectado()) {
        string trama = lectorSerial.recibirTrama();
        
        if (!trama.empty()) {
            clasificador.interpretarTrama(trama);
        }

      
        this_thread::sleep_for(chrono::milliseconds(10));
    }

    return 0;
}