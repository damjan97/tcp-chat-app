# tcp-chat-app

Local TCP terminal chat application made as a school project. Written in C.

Autori: Damjan Glamočić i Mihailo Marković

Prevođenje:

	Server:
		make server
	Klijent:
		make client

Brisanje izvršne datoteke:

	Server:
		make server_clean
	Klijent:
		make client_clean

Pokretanje:

	Server:
		Bez argumenata komandne linije 	
			./server
	Klijent:
		Argument je IP adresa uredjaja na kojem je pokrenut server 
			./client 192.168.0.100
    
Kod logovanja klijenta, potrebno je uneti username po izboru, a kao šifru username123 

		  primer:  Type your username:
			   >> pera
			   Type your password:
			   >> pera123
