#include <sqlite3.h>
#include <iostream>

using namespace std;

int main() {  
    // Función principal del programa, punto de entrada de ejecución.
    
    sqlite3 *db;  
    // Declara un puntero a sqlite3 que representará la base de datos abierta.
    
    int exit = sqlite3_open("BaseDeDatos.db", &db);  
    // Intenta abrir (o crear si no existe) la base de datos "BaseDeDatos.db".
    // El resultado se guarda en 'exit' (0 = éxito, otro valor = error).
    // '&db' pasa la dirección del puntero para que sqlite3_open lo inicialice.

    if(exit) {  
        // Si 'exit' NO es 0, significa que hubo un error al abrir la base de datos.
        cerr << "ERROR AL ABRIR LA BASE DE DATOS: " << sqlite3_errmsg(db) << endl;  
        // Muestra en la salida de errores el mensaje de SQLite correspondiente al error.
        return -1;  
        // Sale del programa indicando que ocurrió un error.
    } else {  
        cout << "BASE DE DATOS ABIERTA CORRECTAMENTE" << endl;  
        // Si 'exit' es 0, la base de datos se abrió correctamente.
    }

    sqlite3_stmt* stmt;  
    // Declara un puntero a un "statement" de SQLite, que contendrá la consulta preparada.

    string sql = "SELECT idMensaje, autor, mensaje, fecha FROM ChatPrueba;";  
    // Define la consulta SQL que vamos a ejecutar: selecciona varias columnas de la tabla ChatPrueba.

    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {  
        // Prepara la consulta SQL para ejecución.
        // db → la base de datos.
        // sql.c_str() → convierte el string C++ a const char* requerido por SQLite.
        // -1 → longitud automática de la cadena SQL.
        // &stmt → dirección del puntero para almacenar el statement preparado.
        // nullptr → no necesitamos almacenar el resto de la cadena SQL.
        // SQLITE_OK = éxito; cualquier otro valor indica error.
        
        cerr << "Error al preparar la consulta: " << sqlite3_errmsg(db) << endl;  
        // Muestra un mensaje de error si la preparación falla.
        return -1;  
        // Sale del programa.
    }

    while (sqlite3_step(stmt) == SQLITE_ROW) {  
        // Ejecuta la consulta fila por fila.
        // sqlite3_step devuelve SQLITE_ROW mientras haya filas disponibles.
        
        int id = sqlite3_column_int(stmt, 0);  
        // Obtiene la primera columna de la fila actual como entero (idMensaje).
        
        const unsigned char* autor = sqlite3_column_text(stmt, 1);  
        // Obtiene la segunda columna (autor) como texto.
        // SQLite devuelve punteros a unsigned char*, por eso usamos este tipo.
        
        const unsigned char* mensaje = sqlite3_column_text(stmt, 2);  
        // Obtiene la tercera columna (mensaje) como texto.
        
        const unsigned char* fecha = sqlite3_column_text(stmt, 3);  
        // Obtiene la cuarta columna (fecha) como texto.

        cout << "ID: " << id 
             << ", Autor: " << autor
             << ", Mensaje: " << mensaje
             << ", Fecha: " << fecha << endl;  
        // Imprime en consola los datos de la fila actual.
        // Nota: el tipo unsigned char* se imprime directamente y C++ lo interpreta como string.
    }

    sqlite3_finalize(stmt);  
    // Libera la memoria y recursos asociados con el statement preparado.
    
    sqlite3_close(db);  
    // Cierra la base de datos y libera recursos asociados al puntero db.

    return 0;  
    // Termina el programa indicando éxito.
}
