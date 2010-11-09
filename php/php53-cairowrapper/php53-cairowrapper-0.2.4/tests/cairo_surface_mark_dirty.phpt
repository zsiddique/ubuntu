--TEST--
cairo_surface_mark_dirty() function
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