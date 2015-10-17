## mongshell

---

mongshell은 일련의 명령어들을 실행해주는 프로그램입니다.
각 명령어는 id를 가지며, piping 기능을 지원합니다.

`id`:`action`:`pipe-id`:`command` 문법으로 명령어들을 실행합니다.


### action

---

`once`, `wait`, `respawn` 타입이 존재합니다.


### piping

---

`pipe-id`가 지정되지 않으면 표준 입출력으로 입출력을 하고, `pipe-id`를 지정하면 해당 명령어로 실행된 프로그램과 piping 됩니다.

### usage

---

터미널에서 `make clean all test` 를 실행하세요.