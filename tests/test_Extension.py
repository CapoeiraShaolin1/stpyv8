#!/usr/bin/env python
# -*- coding: utf-8 -*-

import sys
import unittest
import logging

import STPyV8


class TestExtension(unittest.TestCase):
    def setUp(self):
        self.isolate = STPyV8.JSIsolate()
        self.isolate.enter()

    def tearDown(self):
        self.isolate.leave()
        del self.isolate

    def testExtension(self):
        extSrc = """function hello(name) { return "hello " + name + " from javascript"; }"""
        extJs = STPyV8.JSExtension("hello/javascript", extSrc)

        self.assertTrue(extJs)
        self.assertEqual("hello/javascript", extJs.name)
        self.assertEqual(extSrc, extJs.source)
        self.assertFalse(extJs.autoEnable)
        self.assertTrue(extJs.registered)

        TestExtension.extJs = extJs

        with STPyV8.JSContext(extensions=['hello/javascript']) as ctxt:
            self.assertEqual("hello pythonworld from javascript", ctxt.eval("hello('pythonworld')"))

        # test the auto enable property

        with STPyV8.JSContext() as ctxt:
            self.assertRaises(ReferenceError, ctxt.eval, "hello('pythonworld')")

        extJs.autoEnable = True
        self.assertTrue(extJs.autoEnable)

        with STPyV8.JSContext() as ctxt:
            self.assertEqual("hello pythonworld from javascript", ctxt.eval("hello('pythonworld')"))

        extJs.autoEnable = False
        self.assertFalse(extJs.autoEnable)

        with STPyV8.JSContext() as ctxt:
            self.assertRaises(ReferenceError, ctxt.eval, "hello('pythonworld')")

        extUnicodeSrc = u"""
            function helloW(name) {
                return "hello " + name + " from javascript";
            }
        """
        extUnicodeJs = STPyV8.JSExtension(u"helloW/javascript", extUnicodeSrc)

        self.assertTrue(extUnicodeJs)
        self.assertEqual("helloW/javascript", extUnicodeJs.name)
        self.assertEqual(extUnicodeSrc, extUnicodeJs.source)
        self.assertFalse(extUnicodeJs.autoEnable)
        self.assertTrue(extUnicodeJs.registered)

        TestExtension.extUnicodeJs = extUnicodeJs

        with STPyV8.JSContext(extensions=['helloW/javascript']) as ctxt:
            self.assertEqual("hello pythonworld from javascript", ctxt.eval("helloW('pythonworld')"))

            ret = ctxt.eval(u"helloW('世界')")

            self.assertEqual(u"hello 世界 from javascript", ret )

    def testNativeExtension(self):
        extSrc = "native function hello();"
        extPy = STPyV8.JSExtension("hello/python", extSrc, lambda func: lambda name: "hello " + name + " from python", register=False)
        self.assertTrue(extPy)
        self.assertEqual("hello/python", extPy.name)
        self.assertEqual(extSrc, extPy.source)
        self.assertFalse(extPy.autoEnable)
        self.assertFalse(extPy.registered)
        extPy.register()
        self.assertTrue(extPy.registered)

        TestExtension.extPy = extPy

        with STPyV8.JSContext(extensions=['hello/python']) as ctxt:
            self.assertEqual("hello pythonworld from python", ctxt.eval("hello('pythonworld')"))


if __name__ == '__main__':
    level = logging.DEBUG if "-v" in sys.argv else logging.WARN
    logging.basicConfig(level = level, format = '%(asctime)s %(levelname)s %(message)s')
    unittest.main()
