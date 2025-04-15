"""
Fichero de pruebas funcionales del programa.
Creado por Alberto Penas Díaz

IMPORTANTE: Para que las pruebas funcionen correctamente, es necesario que el servidor esté en ejecución
y que las claves de cliente y servidor coincidan.
"""

import unittest
import random
import subprocess
import re
import sys

SERVER_ADDR = None


class Test(unittest.TestCase):
    server_process = None

    def __run(self, commands):
        """
        Ejecuta la aplicación del cliente con una secuencia de comandos.
        :param commands: Lista de comandos (strings) que se enviarán a la shell.
        :return: Tupla (salida, error) de la ejecución.
        """
        input_data = "\n".join(commands) + "\n"
        process = subprocess.Popen(["env",f"IP_TUPLAS={SERVER_ADDR}", "./cmake-build-release/app-cliente"],
                                   stdin=subprocess.PIPE,
                                   stdout=subprocess.PIPE,
                                   stderr=subprocess.PIPE,
                                   text=True)
        out, err = process.communicate(input_data)
        return out, err

    def test_set_get(self):
        # Prueba de inserción y obtención de una tupla.
        commands = [
            'destroy',
            'set 5 valor_inicial 3 2.3 0.5 23.45 10 5',
            'get 5',
            'exit'
        ]
        out, err = self.__run(commands)
        self.assertIn("Tupla insertada correctamente", out, "La inserción no se realizó correctamente")
        self.assertIn("Obtenido: key=5", out, "La obtención de la tupla no devolvió la clave esperada")

    def test_modify(self):
        # Prueba de modificación de una tupla ya insertada.
        commands = [
            'destroy',
            'set 5 valor_inicial 3 2.3 0.5 23.45 10 5',
            'modify 5 valor_modificado 3 1.0 2.0 3.0 20 10',
            'get 5',
            'exit'
        ]
        out, err = self.__run(commands)
        self.assertIn("Tupla modificada correctamente", out, "La modificación no se realizó correctamente")


    def test_delete_exist(self):
        # Prueba de verificación de existencia y eliminación de una tupla.
        commands = [
            'destroy',
            'set 5 valor_inicial 3 2.3 0.5 23.45 10 5',
            'exist 5',
            'delete 5',
            'exist 5',
            'exit'
        ]
        out, err = self.__run(commands)
        self.assertIn("La clave 5 existe", out, "La clave debería existir después de la inserción")
        self.assertIn("Tupla eliminada correctamente", out, "La eliminación no se realizó correctamente")
        self.assertIn("La clave 5 no existe", out, "La clave debería no existir después de la eliminación")

    def test_destroy(self):
        # Prueba de destrucción del servidor.
        commands = [
            'destroy',
            'exit'
        ]
        out, err = self.__run(commands)
        self.assertIn("Servidor destruido", out, "El servidor no se destruyó correctamente")

    def test_stress_1(self):
        """
        Prueba de estrés 1: Insertar 100 tuplas.
        """
        commands = [
            'destroy',
            *["set {} valor_inicial 3 2.3 0.5 23.45 10 5".format(i) for i in range(100)],
            'exit'
        ]
        out, err = self.__run(commands)

        # Verificamos que todas las inserciones fueron exitosas
        num_inserted = len(re.findall("Tupla insertada correctamente", out))
        self.assertEqual(num_inserted, 100, "No se insertaron todas las tuplas correctamente")

    def test_zzz_concurrency_stress(self):
        """
        Prueba de estrés de concurrencia: Simula múltiples clientes ejecutando operaciones
        concurrentes sobre el servidor.

        ATENCIÓN: SI SE ALTERAN LOS VALORES DE num_clientes Y num_comandos_por_cliente, ES POSIBLE
        QUE ESTE TEST FALLE DEBIDO A QUE EL SERVIDOR NO PUEDE GESTIONAR TANTAS PETICIONES. PARA MAŚ
        INFORMACIÓN, CUNSULTAR EL APARTADO "EVALUACIÓN DE RENDIMIENTO" EN LA MEMORIA DEL PROYECTO.
        """
        from concurrent.futures import ThreadPoolExecutor, as_completed

        num_clientes = 5
        num_comandos_por_cliente = 10
        store =  {}

        # Antes de nada, nos aseguramos que no haya nada en el almacén
        comand = ["destroy", "exit"]
        out, err = self.__run(comand)
        self.assertIn("Servidor destruido", out, "El servidor no se destruyó correctamente")

        def operaciones_cliente(clave):
            comandos = []
            for i in range(num_comandos_por_cliente):
                a = random.random() * 100
                b = random.random() * 100
                c = random.random() * 100

                comandos.append(f'set {clave * num_comandos_por_cliente + i} valor_{clave} 3 {a} {b} {c} 10 5')
                store[clave * num_comandos_por_cliente + i] = (clave, a, b, c)
            for i in range(num_comandos_por_cliente):
                comandos.append(f'get {clave * num_comandos_por_cliente + i}')

            comandos.append("exit")
            salida, error = self.__run(comandos)
            return clave, salida, error

        resultados = []
        with ThreadPoolExecutor(max_workers=num_clientes) as executor:
            futuros = [executor.submit(operaciones_cliente, clave) for clave in range(num_clientes)]
            for futuro in as_completed(futuros):
                clave, salida, error = futuro.result()
                resultados.append((clave, salida, error))

        # Verificamos que todas las operaciones fueron exitosas
        for clave, salida, error in resultados:
            #print(error)
            num_inserted = len(re.findall("Tupla insertada correctamente", salida))
            num_get = len(re.findall("Obtenido: key=", salida))
            self.assertEqual(num_inserted, num_comandos_por_cliente, "No se insertaron todas las tuplas correctamente")
            self.assertEqual(num_get, num_comandos_por_cliente, f"No se obtuvieron todas las tuplas correctamente")

        # Verificamos
        commands = []
        for i in range(num_clientes * num_comandos_por_cliente):
            commands.append(f'get {i}')
        commands.append("exit")

        out, err = self.__run(commands)

        for i in range(num_clientes * num_comandos_por_cliente):
            self.assertIn(f"Obtenido: key={i}, value1=valor_{store[i][0]}", out, "No se obtuvo la tupla correctamente")



    def test_nok_1(self):
        """
        Prueba de error 1: Insertar un vector con más de 1000 elementos.
        """
        commands = [
            'destroy',
            'set 5 key 1000 ' + ' '.join([str(i) for i in range(1000)]),
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("N debe estar entre 1 y 32", out, "El servidor no detectó el error")

    def test_nok2(self):
        """
        Prueba de error 2: Modificar una tupla que no existe.
        """
        commands = [
            'destroy',
            'modify 5 valor_modificado 3 1.0 2.0 3.0 20 10',
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("Error al modificar la tupla", out, "El servidor no detectó el error")

    def test_nok3(self):
        """
        Prueba de error 3: Eliminar una tupla que no existe.
        """
        commands = [
            'destroy',
            'delete 5',
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("Error al eliminar la tupla", out, "El servidor no detectó el error")

    def test_nok4(self):
        """
        Prueba de error 4: Obtener una tupla que no existe.
        """
        commands = [
            'destroy',
            'get 5',
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("Error al obtener la tupla", out, "El servidor no detectó el error")

    def test_nok5(self):
        """
        Tratar de insertar una tupla con una clave que ya existe.
        """
        commands = [
            'destroy',
            'set 5 valor_inicial 3 2.3 0.5 23.45 10 5',
            'set 5 valor_inicial 3 2.3 0.5 23.45 10 5',
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("Error al insertar la tupla", out, "El servidor no detectó el error")

    def test_nok6(self):
        """
        Tratar de insertar una tupla con un vector de tamaño 0 o mayor que 32.
        """
        commands = [
            'destroy',
            'set 5 valor_inicial 0',
            'set 5 valor_inicial 33',
            'exit'
        ]

        out, err = self.__run(commands)

        num_err = len(re.findall("N debe estar entre 1 y 32", out))

        self.assertEqual(num_err, 2, "El servidor no detectó el error")

    def test_nok7(self):
        """
        Tratar de insertar una tupla con un key no numérico
        """
        commands = [
            'destroy',
            'set cadena valor_inicial 3 2.3 0.5 23.45 10 5',
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("La clave debe ser un número entero", out, "El servidor no detectó el error")

    def test_nok8(self):
        """
        Tratar de insertar una tupla con un value1 mayor de 255 caracteres
        """
        commands = [
            'destroy',
            'set 5 ' + 'a'*256 + ' 3 2.3 0.5 23.45 10 5',
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("El valor 1 debe tener como máximo 255 caracteres", out, "El servidor no detectó el error")

    def test_nok9(self):
        """
        Especificar un número diferente de valores de un vector al especificado.
        """
        commands = [
            'destroy',
            'set 5 valor_inicial 3 2.3 0.5 ',
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("Número insuficiente de valores para v2", out, "El servidor no detectó el error")

    def test_nok10(self):
        """
        Poner coordenadas (últimos dos valores) no numéricos
        """
        commands = [
            'destroy',
            'set 5 valor_inicial 3 2.3 0.5 23.45 10 a',
            'exit'
        ]

        out, err = self.__run(commands)

        self.assertIn("Las coordenadas deben ser números enteros", out, "El servidor no detectó el error")

if __name__ == '__main__':
    if len(sys.argv) != 2:
        print("Uso: python3 test.py <IP>")
        sys.exit(1)

    SERVER_ADDR = sys.argv[1]
    # Esto sirve para que el unittest no se confunda, que es un poco bobo
    sys.argv = [sys.argv[0]]

    unittest.main()
