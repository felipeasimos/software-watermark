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
```
number of errors: 11	number of bits: 15
 1(bit_arr[0] = '1'])  ->  2(bit_arr[1] = '0']) 
 2(bit_arr[1] = '0'])  ->  3(bit_arr[2] = '0']) 
 3(bit_arr[2] = '0'])  ->  4(bit_arr[3] = '1'])  1(bit_arr[0] = '1']) 
 4(bit_arr[3] = '1'])  ->  5(bit_arr[4] = '0'])  6[m] <- remoção
 5(bit_arr[4] = '0'])  ->  6[m] 
 6[m]  ->  7(bit_arr[5] = '1']) 
 7(bit_arr[5] = '1'])  ->  8(bit_arr[6] = '0'])  6[m] 
 8(bit_arr[6] = '0'])  ->  9(bit_arr[7] = '1'])  4(bit_arr[3] = '1']) 
 9(bit_arr[7] = '1'])  ->  10(bit_arr[8] = '0'])  11[m] 
 10(bit_arr[8] = '0'])  ->  11[m] 
 11[m]  ->  12(bit_arr[9] = '1']) 
 12(bit_arr[9] = '1'])  ->  13(bit_arr[10] = '0'])  1(bit_arr[0] = '1']) 
 13(bit_arr[10] = '0'])  ->  14(bit_arr[11] = '1']) 
 14(bit_arr[11] = '1'])  ->  15(bit_arr[12] = '0'])  1(bit_arr[0] = '1']) 
 15(bit_arr[12] = '0'])  ->  16(bit_arr[13] = '1']) 
 16(bit_arr[13] = '1'])  ->  17(bit_arr[14] = '0'])  1(bit_arr[0] = '1']) 
 17(bit_arr[14] = '0'])  ->  null 
 null  ->  null 
 null 
 1(bit_arr[0] = '1'])  ->  2(bit_arr[1] = '0']) 
 2(bit_arr[1] = '0'])  ->  3(bit_arr[2] = '0']) 
 3(bit_arr[2] = '0'])  ->  4(bit_arr[3] = '0'])  1(bit_arr[0] = '1']) 
 4(bit_arr[3] = '0'])  ->  5(bit_arr[4] = '0']) 
 5(bit_arr[4] = '0'])  ->  6(bit_arr[5] = '0']) 
 6(bit_arr[5] = '0'])  ->  7(bit_arr[6] = '1']) 
 7(bit_arr[6] = '1'])  ->  8(bit_arr[7] = '0'])  6(bit_arr[5] = '0']) 
 8(bit_arr[7] = '0'])  ->  9(bit_arr[8] = '1'])  4(bit_arr[3] = '0']) 
 9(bit_arr[8] = '1'])  ->  10(bit_arr[9] = '0'])  11[m] 
 10(bit_arr[9] = '0'])  ->  11[m] 
 11[m]  ->  12(bit_arr[10] = '1']) 
 12(bit_arr[10] = '1'])  ->  13(bit_arr[11] = '0'])  1(bit_arr[0] = '1']) 
 13(bit_arr[11] = '0'])  ->  14(bit_arr[12] = '1']) 
 14(bit_arr[12] = '1'])  ->  15(bit_arr[13] = '0'])  1(bit_arr[0] = '1']) 
 15(bit_arr[13] = '0'])  ->  16(bit_arr[14] = '1']) 
 16(bit_arr[14] = '1'])  ->  17(bit_arr[15] = '0'])  1(bit_arr[0] = '1']) 
 17(bit_arr[15] = '0'])  ->  null 
 null  ->  null 
 null 
```
