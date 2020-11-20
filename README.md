# Тестовое задание для работодателя. Асинхронный сервер запросов.

Разработать консольное серверное приложение работающее в среде ОС LINUX
на пользовательском уровне. Приложение должно генерировать
последовательность целых чисел, состоящую из 3-х подпоследовательностей,
каждая из которых представляет собой целочисленный неотрицательный 64-х
битный счетчик. Для каждой такой подпоследовательности начальное
значение и шаг между двумя соседними значениями задается пользователем
произвольно.

Формат задания параметров – простой текст в tcp/ip сокет (для проверки
используется telnet-клиент).

## Перечень команд

- seq1 xxxx yyyy (задать начальное значение = xxxx и шаг = yyyy для
  первой подпоследовательности);
- seq2 xxxx yyyy (задать начальное значение = xxxx и шаг = yyyy для
  второй подпоследовательности);
- seq3 xxxx yyyy (задать начальное значение = xxxx и шаг = yyyy для
  третьей подпоследовательности);
- export seq - выдавать в сокет каждому клиенту сгенерированную
  последовательность.

## Примечания

- Если в командах 1, 2, 3 любой из параметров (начальное значение и/или
  шаг) будет указан как = 0, то программа не должна учитывать данную
  подпоследовательность;
- При переполнении счетчика подпоследовательность должна начинаться сначала;
- Формат передаваемых по сети данных – 64-х битные целые числа (binary
  data);
- Программа не должна аварийно завершать работу в случаях некорректно
  введенных параметров, аварийного завершения работы клиента и т.д;
- Язык для разработки – C, компилятор GCC, в проекте должен
  присутствовать Makefile;

- Для создания/управления потоками, списками, примитивами синхронизации
  и т.п. можно использовать сторонние библиотеки.

## Примеры входных и выходных данных

``` 
seq1 1 2 – задает подпоследовательность 1, 3, 5 и т.д;
seq2 2 3 – задает подпоследовательность 2, 5, 8 и т.д;
seq3 3 4 – задает подпоследовательность 3, 7, 11 и т.д;
export seq – в сокет передается последовательность 1, 2, 3, 3, 5, 7,
   5, 8, 11 и т.д.
```

Срок выполнения – 5 рабочих дней. Если будут какие-либо вопросы, их
можно задавать в почте.

## Замечания по реализации.

Для удобства отладки я изменил формат команд и добавил команду export bin.

```
export seq - в сокет передается последовательность чисел в ASCII
export bin - в сокет передается последовательность чисел в виде 64-битных
             чисел в процессорном порядке бит.
```

Я реализовал сервер в ввиде однопоточнго асинхронного приложения с
использованием библиотеки libevent2. Для сборки приложения нужно
установить пакет libevent-dev. Компилятор С можно использовать любой, я
оттестировал сборку компиляторами gcc, clang и Intel C. По умолчанию,
используется компилятор gcc.

Для сборки в директории с исходными текстами необходимо запустить команду make

```
tar xvfz seqserver.tar.gz
cd seqserver
make
```

## Запуск программы выполняется командой:

```
seqserver <port>
```

<port> - номер порта, на котором сервер ждет входящие соединения.

