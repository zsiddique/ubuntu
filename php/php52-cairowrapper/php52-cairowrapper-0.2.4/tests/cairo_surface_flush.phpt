--TEST--
cairo_surface_flush() function
--SKIPIF--
<?php 

if(!extension_loaded('cairo_wrapper')) die('skip ');

 ?>
--FILE--
<?php
echo 'OK'; // no test case for this function yet
?>
--EXPECT--
OK