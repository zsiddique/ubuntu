--TEST--
cairo_clip_extents() function
--SKIPIF--
<?php 

if(!extension_loaded('cairo_wrapper')) die('skip ');

if(!function_exists('cairo_clip_extents')) die('skip not compiled in (CAIRO_VERSION_MAJOR*100+CAIRO_VERSION_MINOR >= 104)');

 ?>
--FILE--
<?php
echo 'OK'; // no test case for this function yet
?>
--EXPECT--
OK