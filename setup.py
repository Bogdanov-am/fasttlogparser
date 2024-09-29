from setuptools import setup, Extension
import pybind11

cpp_args = ['-std=c++11', '-stdlib=libc++', '-mmacosx-version-min=10.7']

sfc_module = Extension(
    'fasttlogparser',
    sources=[
        'src/module.cpp',
        'src/MessageSeries.cpp',
        ],
    include_dirs=[
        'src',
        pybind11.get_include(),
        'C:/projects/mavlink/out'
        ],
    language='c++',
    extra_compile_args=cpp_args,
)

setup(
    name='fasttlogparser',
    version='1.0',
    description='Python package with fasttlogparser C++ extension (PyBind11)',
    ext_modules=[sfc_module],
)