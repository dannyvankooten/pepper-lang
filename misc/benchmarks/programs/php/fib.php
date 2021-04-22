<?php

function fib($x) {
	if ($x < 2) {
		return $x;
	}

	return fib($x - 1) + fib($x - 2);
}

echo "35th fibonacci number is: ", fib(35);
