# Watermark

Reed-Solomon library copied from: https://github.com/tierney/reed-solomon.

## Djikstra

* Toda marca d'agua é um grafo de djiktra e todo o grafo de djikstra que não
tenha estruturas case e if then else, é uma marca d'água válida.
	* podemos comparar grafos de djikstra ao comparar os bit sequence que
	eles representam.
* expansão do gráfico: adicionar comandos estruturados.

## TODO

* achar código de teste.

* main completo

* comparação: código de teste com marca d'agua embarcada dentro de uma funcão como
código morto (dummy) e código embarcado (integrado com o grafo existente do código)
	* comparar complexidade aciclomática
	* comparar código morto (sonarqube)

* i < 100
    * 65% de erro (por combinações totais)
    * 98% de erro (por número) (2 e 14 dão certo)
* 10⁷ < i < 10⁸
    * 55% de error (por combinações totais)
    * 100% de erro (por número)

## Debug Notes

65582 with errors:
```
 1  ->  2 
 2  ->  3 
 3  ->  4  1 
 4  ->  5 
 5  ->  6  1 
 6  ->  7 
 7  ->  8  1 
 8  ->  9 
 9  ->  10  1 
 10  ->  11 
 11  ->  12  1 
 12  ->  13  14 
 13  ->  14 
 14  ->  15 
 15  ->  16  12 
 16  ->  17  1 
 17  ->  18  19 
 18  ->  19 
 19  ->  20 
 20  ->  21  19 
 21  ->  22  1 
 22  ->  23  24 
 23  ->  24 
 24  ->  25 
 25  ->  26  24 
 26  ->  27  22 
 27  ->  28  1 
 28  ->  29  30 
 29  ->  30 
 30  ->  31 
 31  ->  32  1 
 32  ->  33 
 33  ->  34  1 
 34  ->  35 
 35  ->  36  1 
 36  ->  37 
 37  ->  38  1 
 38  ->  39  40 
 39  ->  40 
 40  ->  41 
 41  ->  42  38 
 42  ->  43 
 43  ->  44  42 
 44  ->  45  38 
 45  ->  46  1 
 46  ->  47  48 
 47  ->  48 
 48  ->  49 
 49  ->  50  1 
 50  ->  51 
 51  ->  52  1 
 52  ->  53 
 53  ->  54  1 
 54  ->  55 
 55  ->  56  1 
 56  ->  57 
 57  ->  58  1 
 58  ->  59  60 
 59  ->  60 
 60  ->  61 
 61  ->  62  60 
 62  ->  63  1 
 63  ->  64  65 
 64  ->  63 
 65  ->  66 
 66  ->  67 
 67  ->  68  63 
 68  ->  69 
 69  ->  70  1 
 70  ->  71 
 71  ->  72  1 
 72  ->  73 
 73  ->  null  1 
 null  ->  null 
 null 
```
65582 without errors:
```
 1  ->  2 
 2  ->  3 
 3  ->  4  1 
 4  ->  5 
 5  ->  6  1 
 6  ->  7 
 7  ->  8  1 
 8  ->  9 
 9  ->  10  1 
 10  ->  11 
 11  ->  12  1 
 12  ->  13  14 
 13  ->  14 
 14  ->  15 
 15  ->  16  12 
 16  ->  17  1 
 17  ->  18  19 
 18  ->  19 
 19  ->  20 
 20  ->  21  17 
 21  ->  22  1 
 22  ->  23  24 
 23  ->  24 
 24  ->  25 
 25  ->  26  24 
 26  ->  27  22 
 27  ->  28  1 
 28  ->  29  30 
 29  ->  30 
 30  ->  31 
 31  ->  32  1 
 32  ->  33 
 33  ->  34  1 
 34  ->  35 
 35  ->  36  1 
 36  ->  37 
 37  ->  38  1 
 38  ->  39  40 
 39  ->  40 
 40  ->  41 
 41  ->  42  40 
 42  ->  43  38 
 43  ->  44  45 
 44  ->  45 
 45  ->  46 
 46  ->  47  38 
 47  ->  48  49 
 48  ->  49 
 49  ->  50 
 50  ->  51  38 
 51  ->  52  1 
 52  ->  53 
 53  ->  54  1 
 54  ->  55 
 55  ->  56  1 
 56  ->  57 
 57  ->  58  1 
 58  ->  59 
 59  ->  60  58 
 60  ->  61 
 61  ->  62  58 
 62  ->  63  1 
 63  ->  64  65 
 64  ->  63 
 65  ->  66 
 66  ->  67 
 67  ->  68  1 
 68  ->  69 
 69  ->  70  1 
 70  ->  71 
 71  ->  72  1 
 72  ->  73 
 73  ->  null  1 
 null  ->  null 
 null 
```
