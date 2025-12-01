#include <sqlite3.h>      // Incluye la librería de SQLite para manejar bases de datos.
#include <iostream>       // Incluye la librería de entrada/salida de C++ (cout, cerr, etc.).
#include <string>         // Incluye la librería de strings de C++.

using namespace std;      // Para no tener que poner "std::" delante de cout, string, cerr, etc.

int main() {
    sqlite3* db;  // Declara un puntero a la base de datos SQLite. Será usado para abrir y manipular la DB.

    // Abre la base de datos "BaseDeDatos.db". Si no existe, SQLite intentará crearla.
    // 'exit' recibirá 0 si la apertura fue correcta, cualquier otro valor indica error.
    int exit = sqlite3_open("BaseDeDatos.db", &db);

    if(exit) {  // Si exit != 0, hubo un error al abrir la base de datos
        cerr << "ERROR AL ABRIR LA BASE DE DATOS: " << sqlite3_errmsg(db) << endl; 
        // sqlite3_errmsg(db) devuelve un mensaje descriptivo del error.
        return -1;  // Termina el programa con código de error
    }

    // Definimos la sentencia SQL para insertar un mensaje
    // Nótese que NO incluimos 'idMensaje'; SQLite lo generará automáticamente por AUTOINCREMENT.
    string sql = "INSERT INTO ChatPrueba (autor, mensaje, fecha) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt; // Declaramos un puntero a la sentencia preparada

    // Preparamos la sentencia SQL para que SQLite la ejecute
    // sqlite3_prepare_v2 compila la consulta SQL y la guarda en 'stmt'
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        cerr << "Error al preparar la consulta: " << sqlite3_errmsg(db) << endl;
        return -1;  // Si hay error en la preparación, se termina el programa
    }
    // Asignamos valores a los placeholders '?' en la sentencia preparada
    // El primer parámetro es la posición (1-indexed)
    sqlite3_bind_text(stmt, 1, "Alvaro", -1, SQLITE_STATIC);  
    // autor: "Gonzalo"
    sqlite3_bind_text(stmt, 2, "usuario", -1, SQLITE_STATIC);  
    // mensaje: "Hola, este es un mensaje"
    sqlite3_bind_text(stmt, 3, "2025-11-20", -1, SQLITE_STATIC);  
    // fecha: "2025-11-19"
    // Ejecutamos la sentencia
    // sqlite3_step devuelve SQLITE_DONE si la ejecución fue correcta
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        cerr << "Error al insertar: " << sqlite3_errmsg(db) << endl;  
        // Muestra mensaje de error si no se pudo insertar
    } else {
        cout << "Mensaje insertado correctamente" << endl;  
        // Mensaje de éxito
    }

    sqlite3_finalize(stmt);  // Libera la memoria usada por la sentencia preparada
    sqlite3_close(db);       // Cierra la base de datos

    return 0;  // Termina el programa correctamente
}
