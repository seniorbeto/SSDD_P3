# Ejercicio 2
**Sistemas Distribuidos**

**Alberto Penas Díaz**

NIA: 100471939

Correo: 100471939@alumnos.uc3m.es 
Titulación: Ingeniería informática

Alberto Penas Díaz.
# <a name="_page1_x43.65_y98.82"></a>**Índice**

- [Instrucciones de Compilación y Ejecución](#instrucciones-de-compilación-y-ejecución)
- [Diseño del Programa](#diseño-del-programa)
    - [Diseño General](#diseño-general)
    - [Diseño del Protocolo](#diseño-del-protocolo)
- [Batería de Pruebas](#batería-de-pruebas)
- [Evaluación de rendimiento](#evaluación-de-rendimiento)
- [Contenido Extra](#contenido-extra)
    - [Terminal del Cliente](#terminal-del-cliente)
    - [Servicio Criptográfico XTEA](#servicio-criptográfico-xtea)
    - [Otras Mejoras](#otras-mejoras)

# **Instrucciones de Compilación y Ejecución**
Para compilar y ejecutar el programa se ha usado la herramienta CMake (disponible en Guernika). Se ha creado un script en bash para automatizar la compilación. Este script se llama **compile.sh** y se encarga  de  crear  todas  las  librerías  y  ejecutables  del  proyecto.  Cabe  destacar  que  en el fichero CMakeLists.txt se vinculan directamente las librerías libclaves.s, libencript.so y libserialitation.so con el cliente.  Para  ejecutar  este  script  de  compilación,  primero  de  todo  hay  que  otorgarle permisos de ejecución con: chmod +x ./compile.sh Posteriormente, se puede ejecutar el script de compilación con ./compile.sh Este script generará el directorio *cmake-build-release/* en el que se encuentran todos los ejecutables. Al contrario que la práctica anterior, el servidor requiere como argumento de entrada el puerto sobre el que escuchará peticiones y el cliente necesita que haya declaradas las variables de entorno PORT\_TUPLAS e IP\_TUPLAS. Ambos se pueden ejecutar de la siguiente manera

\>> ./cmake-build-release/servidor-mq 4444 > server.txt &[^1]

\>>  env  PORT\_TUPLAS=4444  IP\_TUPLAS=localhost ./cmake-build-release/app-cliente

Para  la  batería  de pruebas, simplemente es necesario ejecutar el fichero de python test.py desde terminal, especificando como argumentos la ip y puerto. Antes de ejecutarlo, es muy importante tener el proceso del servidor ejecutándose al mismo tiempo, si no, los test no pasarán.

\>> ./cmake-build-release/servidor-mq 4444 & >> python3 test.py localhost 4444
# Diseño General
## Diseño del Programa
La arquitectura principal es idéntica a la de la práctica anterior pero el proceso de comunicación se establece, esta vez a través de Sockets TCP. Si bien se especifica en el enunciado que se ha de utilizar este protocolo de transporte, su uso es mucho más que conveniente en este escenario debido a que es un protocolo orientado a la conexión, permitiendo una mucho mejor gestión en temas de comunicación y protocolos de encriptado. Si bien no es tan rápida como UDP, bajo el contexto de esta práctica, no resulta tan interesante la velocidad y eficiencia del servidor tanto como la consistencia del mismo.

Además, para ordenar la estructura del proyecto, se han incluído dos bibliotecas extra:

- libserialitation.so: librería que se encarga de serializar y deserializar peticiones y respuestas, común tanto al cliente como al servidor
- lines.c:  conjunto  de  funciones  auxiliares  (otorgadas  en  los  ejercicios  de  la asignatura) que permiten asegurar la lectura y escritura de todos los bytes introducidos como argumento en el socket.
## Diseño del Protocolo
Para que el protocolo de comunicación fuera totalmente ajeno al lenguaje de programación en el que se ejecuta la aplicación, se ha utilizado una serialización común en los mensajes transmitidos entre cliente y servidor. Esta estrategia de serialización es **XML**, ya que su gran uso y estandarización han resultado relevantes a la hora de implementar su uso en el código. Usando XML, no hay problema con respecto al formato de número, ya que el string en ASCII es completamente legible independientemente de la máquina. **Este formato de serialización formatea tal cual las estructuras que se han usado para la práctica  anterior**  (request\_t  y  response\_t)  ya  que  seguir  usando  esas  estructuras  permite  una conceptualización del código más limpia y estructurada.

En cuanto al protocolo de comunicación como tal, se podría esquematizar en el siguiente diagrama:

<img src="docs/imgs/Aspose.Words.8b3b0b35-28b5-40dd-bd8a-5b5fa59c73e6.005.png"/>

Como se puede observar, el lado del servidor (izquierda) crea un nuevo socket (en un hilo[^2] separado) por  cada  petición  nueva,  para  poder  seguir  aceptando  peticiones  en  el  socket (hilo) principal. Es importante mencionar que **la única parte que NO está serializada con XML es la parte en la que se envía  el tamaño del mensaje que se va a enviar**. Este número es un entero sin signo de 32 bits y se envía por red utilizando la función *host to network* por lo que, aún siendo un entero, cualquier máquina es capaz de reconocerlo. También es importante mencionar que este número se manda sin encriptar.

Como  mención  importante,  la  decisión de diseño de enviar previamente el tamaño del cuerpo del mensaje que se va a enviar no ha sido tomada a la ligera. Se ha implementado porque, si bien se hubiera podido enviar la cadena de caracteres ASCII y leer del socket hasta el caracter “\0”, esto **NO ES POSIBLE** si se encripta la cadena, puesto que ese caracter (\0) puede ser que se encuentre en el medio de la cadena (es totalmente imposible determinarlo). Es por ello que es necesario saber de antemano cuántos Bytes habrá que leer del socket para saber con exactitud qué se debe desencriptar.
# Batería de Pruebas
Para la batería de pruebas se ha utilizado Unitest en Python. Debido al límite de 5 páginas en la memoria, se han decidido explicar todas las pruebas en profundidad en el propio archivo test.py que recoge todas las pruebas funcionales del programa, en donde se incluyen varios test que ponen a prueba la concurrencia del servidor.
# Evaluación de rendimiento
El comportamiento usando sockets es abruptamente distinto al uso de colas de mensajes. En un primer lugar, hay que mencionar que, al igual que las colas de mensajes, la capacidad de rendimiento del servidor depende total y absolutamente de la cantidad de mensajes recibidos que puede almacenar en la  recepción  del  socket  en  modo  escucha.  En  un  principio,  se  especificaron  10  mensajes  ( *listen(server\_sock, 10)* en servidor-sock.c), lo que hacía que, al estresar el servidor ocurrieran múltiples errores del tipo: *Connection reset by peer*, lo que indicaba un cierre abrupto del socket al que se había conectado el cliente. Esto es muy fácil de solucionar incrementando el número de peticiones que puede almacenar el servidor ya que, a diferencia de las colas de mensajes, no está limitado a 10 unidades. Modificando este parámetro a 128 se obtienen los siguientes resultados ejecutando cliente y servidor en una **máquina local**:

![](docs/imgs/Aspose.Words.8b3b0b35-28b5-40dd-bd8a-5b5fa59c73e6.006.jpeg)

Es  sorprendente  observar  cómo  al  ejecutarlo  en  una  máquina  local  se  obtiene  un  rendimiento del orden de  100 veces superior a ejecutar ambos  procesos  en  máquinas  distintas.  Como dato curioso, para realizar estas  pruebas, se ha tenido que desactivar  la  protección  contra  ataques  DDOS  del  router,  ya  que  estas pruebas de  estrés las interpretaba como tal.

Por  último,  se  ha  analizado  con  un  analizador  de  paquetes  de  red  (la  herramienta  de  hacking Wireshark) toda la información de red que se transmitía en una comunicación cliente y servidor:

<img src="docs/imgs/Aspose.Words.8b3b0b35-28b5-40dd-bd8a-5b5fa59c73e6.008.jpeg"/>

Es muy complicado y denso explicar todo el alcance que tiene utilizar en esta práctica un analizador de paquetes de red, por lo que se ha realizado un vídeo en el que se muestra más en detalle cómo se puede utilizar esta herramienta para analizar toda la comunicación entre cliente y servidor y cómo se pueden observar los mensajes en texto plano si estos no se encriptan:

<https://youtu.be/btGp2FSTEZQ>
# Contenido Extra
## Terminal del Cliente
Para acceder a las funciones de claves.c por parte del cliente, se ha realizado una “pseudoterminal” desde la cual el cliente puede emplear de forma directa las funciones de la librería. Para mejorar el aspecto visual de esta terminal, se han incluído colores, mensajes de error significativos y la capacidad de ejecutarse sin la necesidad de que esté el servidor en ejecución.

## Servicio Criptográfico XTEA

Para una mayor seguridad (y pensando en la siguiente práctica en la que se usan sockets) toda la comunicación entre cliente y servidor se cifra mediante el algoritmo de criptografía simétrica XTEA. Como en cualquier otro algoritmo de criptografía simétrica, tanto cliente y servidor deben conocer la clave (unsigned de 128 bits) para encriptar las estructuras de comunicación (*request\_t y response\_t*) por bloques de 8 bytes y 64 rondas. Si la clave de encriptado no coincide, el primer error ocurrirá cuando el servidor trate de obtener el nombre de la cola del cliente, ya que tratará de acceder a una región  de  la  estructura  que  ha  sido  incorrectamente  desencriptada  y por tanto, contendrá valores prácticamente aleatorios.

Con respecto a la práctica anterior, la librería de encriptado ha supuesto un reto significativo. En primer lugar, por lo ya comentado en el Diseño del Protocolo, pero además por la significativa latencia que introducía en la comunicación del cliente y servidor. Aparte de esto, también ha sido necesario modificar la librería para incluir un *padding* en la secuencia de bytes a encriptar. Esto no había sido necesario en la práctica anterior porque las estructuras en C definidas en la práctica anterior ocupan un número de Bytes en memoria que siempre es múltiplo de 8, cosa que no puede pasar con una secuencia de caracteres. Como el tamaño de bloque del algoritmo XTEA es 8, si el mensaje no es múltiplo de éste número, estamos encriptando bytes completamente aleatorios, que influyen de manera destructiva en lo que se está encriptando. Incluyendo  un padding, forzamos a que toda cadena de Bytes que se quiera encriptar ha de ser múltiplo de 8.

Para una mejor depuración del código (y para poder ver en Wireshark las trazas de red) se ha incluído un flag de depuración en la librería llamado DEBUG. Activando este flag, se desactiva temporalmente la librería de encriptado y los mensajes se transmiten en texto plano.
## Otras Mejoras
Como otras mejoras y para probar la solidez y consistencia del código, se han tenido en cuenta y se han corregido todos los warnings posibles, declarando en el CMakeLists.txt:  *add\_compile\_options(-Wall -Wextra -Werror -pedantic -pedantic-errors -Wconversion -Wsign-conversion -pthread -fPIC)*. También se ha utilizado la herramienta clangformat para formatear el código a un estilo consistente y se ha empleado el controlador de versiones de GitHub.
