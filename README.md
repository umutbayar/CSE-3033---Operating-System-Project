Projede bulunan sys/wait.h kütüphanesini windows desteklemiyor. Bu yüzden linux tabanlı bir işletim sistemi kullanmak zorundayız. Linuxda c çalıştırmak için " sudo apt-get install gcc " komutunu konsola girin. (kontrol için " gcc --version ")
Ardından vs code veya nano, vim tarzı derleyici kullanıp çalıştırın. Run ederek çalışmazsa konsoldan gcc mainSetup.c -0 mainSetup komutu çalıştırın sonra bunu runlarız (./mainSetup)

Güncel alınan 5 hatanın geçmesi için soldaki dosyalarda oluşan .vscode klasörü içine c_cpp_properties.json adında bir dosya oluşturulmuş olmalı, onun içerisine 
    "configurations": [
        {
            "name": "Linux",
            "includePath": [
                "${workspaceFolder}/**"
            ],
            "defines": [
                "__USE_POSIX",
                "_GNU_SOURCE"
            ],
            "compilerPath": "/usr/bin/gcc",
            "cStandard": "c99",
            "cppStandard": "c++17",
            "intelliSenseMode": "gcc-x64"
        }
    ],
    "version": 4
}
bunu yapıştırın. Geçmesi gerekli 
**** sample projectsdeki mainSetup1 bakacağımız kod. 


PROJECT 3 çalıştırılması ile alakalı.
thread kütüphanesi yine windowsda çalışmayan bir kütüphane linux üzerinden çalıştırdıktan sonra çalıştırmak için gcc -o main main.c -pthread -lm yazın
ardından ./main 1 100 4 1 gibi 4 input girecek şekilde çalıştırın .
