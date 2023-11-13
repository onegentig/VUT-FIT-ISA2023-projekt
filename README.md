# ISA PROJEKT, FIT VUT 2023 #

*Projekt (TFTP klient a server) z predmetu Sieťové aplikácie a správa sietí (ISA), tretí semester bakalárskeho štúdia BIT na FIT VUT/BUT, ak.rok 2023/2024*

🔒 **Aktívny súkromný repozitár — nezverejňovať!**
<!-- 🗄️ **Súkromný archivovaný repozitár!** -->
<!-- ⚠️ **Zverejnené pre archívne účely — nekopírujte, nula by Vás mrzela. Za nič také nenesiem žiadnu zodpovednosť!** Všetky odovzdané projekty prechádzajú kontrolou plagiátorstva, pri ktorej sa porovnávajú aj s dávnejšie odovzdanými riešeniami. -->
<br />

Hodnotenie: ?? / 20<br />（?）

Zadanie: [ZADANI.md](ZADANI.md), originál [STUDIS](https://www.vut.cz/studis/student.phtml?script_name=zadani_detail&apid=268266&zid=54264)

### TODO-List ###

- [X] ⏰ 2023-09-05 **Zadanie**
- [X] ⏰ 2023-09-06 **Registrácia**
- [X] Vytvoriť triedy pre packety
- [X] Vytvoriť `server/main.cpp`
- [X] Vytvoriť `TFTPServer` a záchyt packetov
- [X] Vytvoriť `TFTPServerConnection` a TID záchyt packetov
- [X] V `TFTPServerConnection` implementovať upload flow (`RRQ`)
- [ ] V `TFTPServerConnection` implementovať download flow (`WRQ`)
- [ ] `TFTPServer` graceful shutdown
- [ ] Vytvoriť `client/main.cpp`
- [ ] Vytvoriť `TFTPClient`
- [ ] V `TFTPClient` implementovať upload flow (`WRQ`)
- [ ] V `TFTPClient` implementovať download flow (`RRQ`)
- [ ] Vytvoriť `PacketLogger` podľa zadania, normalizovať všetky logy
- [ ] Nahradiť multi-threading na niečo iné (select, poll, epoll, …)
- [ ] …RFC rozšírenia…
- [ ] ⏰ 2023-11-20 **Deadline**

### Môže sa hodiť ###

- [TFTP Protocol Standard](https://datatracker.ietf.org/doc/html/rfc1350)
- [What is TFTP? on GeeksforGeeks](https://www.geeksforgeeks.org/what-is-tftp-trivial-file-transfer-protocol/)
- [TFTP on Wikipedia](https://en.wikipedia.org/wiki/Trivial_File_Transfer_Protocol)
- [Hands-On Network Programming with C](https://ebookcentral.proquest.com/lib/vutbrno/reader.action?docID=5774233)
  - [Code Repository](https://github.com/codeplea/hands-on-network-programming-with-c)
- [UNIX Network Programming](https://libgen.rs/book/index.php?md5=7455B393ADC934D8F0D5BA941CADC47D)
- [Socket-Programming Fundamentals in C by Think and Learn](https://youtu.be/_lQ-3S4fJ0U?list=PLPyaR5G9aNDvs6TtdpLcVO43_jvxp4emI)
- [C++ Socket-Programming on GeeksforGeeks](https://www.geeksforgeeks.org/socket-programming-cc/)

----------------------------------------------

<div align="center"><a href="https://wakatime.com"><img alt="wakatime" height="20em" src="https://wakatime.com/badge/user/dd421270-8f1c-43aa-aa5b-ec52a2a18852/project/cec5aeb3-ca5f-4d57-a522-6de66d9ce6bf.svg?style=for-the-badge" /></a></div>
