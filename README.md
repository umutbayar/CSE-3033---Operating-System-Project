Projede bulunan sys/wait.h kütüphanesini windows desteklemiyor. Bu yüzden linux tabanlı bir işletim sistemi kullanmak zorundayız. Linuxda c çalıştırmak için " sudo apt-get install gcc " komutunu konsola girin. (kontrol için " gcc --version ")
Ardından vs code veya nano, vim tarzı derleyici kullanıp çalıştırın. Run ederek çalışmazsa konsoldan gcc mainSetup.c -0 mainSetup komutu çalıştırın sonra bunu runlarız (./mainSetup)