#include <pybind11/pybind11.h>
#include <pybind11/numpy.h>
#include <cinttypes>
#include <stdexcept>

namespace py = pybind11;

class JaggedArraySrc {
private:
    static std::uint16_t swap_uint16(std::uint16_t val) {
        return (val << 8) | (val >> 8);
    }

    static std::int16_t swap_int16(std::int16_t val) {
        return (val << 8) | ((val >> 8) & 0xFF);
    }

    static std::uint32_t swap_uint32(std::uint32_t val) {
        val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
        return (val << 16) | (val >> 16);
    }

    static std::int32_t swap_int32(std::int32_t val) {
        val = ((val << 8) & 0xFF00FF00) | ((val >> 8) & 0xFF00FF);
        return (val << 16) | ((val >> 16) & 0xFFFF);
    }

    static std::int64_t swap_int64(std::int64_t val) {
        val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
        val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
        return (val << 32) | ((val >> 32) & 0xFFFFFFFFULL);
    }

    static std::uint64_t swap_uint64(std::uint64_t val) {
        val = ((val << 8) & 0xFF00FF00FF00FF00ULL) | ((val >> 8) & 0x00FF00FF00FF00FFULL);
        val = ((val << 16) & 0xFFFF0000FFFF0000ULL) | ((val >> 16) & 0x0000FFFF0000FFFFULL);
        return (val << 32) | (val >> 32);
    }

    template <typename T>
    static void makeNative(py::array_t<T> input) {
        py::buffer_info array_info = input.request();
        std::string format = array_info.format;
        if (format.at(0) != '>' && format.at(0) != '<') {
            return;
        }

        auto array_ptr = (T*)array_info.ptr;
        int N = array_info.shape[0] / array_info.itemsize;

        if (format.at(1) == 'H') {
            for (ssize_t i = 0; i < array_info.size; i++) {
                array_ptr[i * N] = (T)swap_uint16((std::uint16_t)array_ptr[i * N]);
            }
            return;
        }
        if (format.at(1) == 'h') {
            for (ssize_t i = 0; i < array_info.size; i++) {
                array_ptr[i * N] = (T)swap_int16((std::int16_t)array_ptr[i * N]);
            }
            return;
        }
        if (format.at(1) == 'L') {
            for (ssize_t i = 0; i < array_info.size; i++) {
                array_ptr[i * N] = (T)swap_uint32((std::uint32_t)array_ptr[i * N]);
            }
            return;
        }
        if (format.at(1) == 'l') {
            for (ssize_t i = 0; i < array_info.size; i++) {
                array_ptr[i * N] = (T)swap_int32((std::int32_t)array_ptr[i * N]);
            }
            return;
        }
        if (format.at(1) == 'Q') {
            for (ssize_t i = 0; i < array_info.size; i++) {
                array_ptr[i * N] = (T)swap_uint64((std::uint64_t)array_ptr[i * N]);
            }
            return;
        }
        if (format.at(1) == 'q') {
            for (ssize_t i = 0; i < array_info.size; i++) {
                array_ptr[i * N] = (T)swap_int64((std::int64_t)array_ptr[i * N]);
            }
            return;
        }
        throw std::invalid_argument("Byteswap is not supported for this type");
        return;
    }

public:
    py::array_t<std::int64_t> starts,
                              stops;
    py::array                 content_array;
    JaggedArraySrc*           content_jagged;
    char                      content_type;
    ssize_t                   iter_index;
    /**************************************************************************
    These are the content_type character codes:
    'a' = array
    'j' = JaggedArray
    't' = table
    **************************************************************************/

    char get_content_type() { return content_type; }

    py::array get_content_array() {
        if (content_type != 'a') {
            throw std::invalid_argument("JaggedArray must be of 'array' content type");
        }
        return content_array;
    }

    void set_content_array(py::array content_array_) {
        if (content_type != 'a') {
            throw std::invalid_argument("JaggedArray must be of 'array' content type");
        }
        content_array = content_array_;
    }

    JaggedArraySrc* get_content_jagged() {
        if (content_type != 'j') {
            throw std::invalid_argument("JaggedArray must be of 'jagged' content type");
        }
        return content_jagged;
    }

    void set_content_jagged(JaggedArraySrc* content_jagged_) {
        if (content_type != 'j') {
            throw std::invalid_argument("JaggedArray must be of 'jagged' content type");
        }
        content_jagged = content_jagged_;
    }

    py::array_t<std::int64_t> get_starts() { return starts; }

    template <typename T>
    void set_starts(py::array_t<T> starts_) {
        makeNative(starts_);
        py::buffer_info starts_info = starts_.request();
        if (starts_info.ndim < 1) {
            throw std::domain_error("starts must have at least 1 dimension");
        }
        int N = starts_info.strides[0] / starts_info.itemsize;
        auto starts_ptr = (T*)starts_info.ptr;
        for (ssize_t i = 0; i < starts_info.size; i++) {
            if (starts_ptr[i * N] < 0) {
                throw std::invalid_argument("starts must have all non-negative values: see index [" + std::to_string(i * N) + "]");
            }
        }
        starts = (py::array_t<std::int64_t>)starts_;
    }

    py::array_t<std::int64_t> get_stops() { return stops; }

    template <typename T>
    void set_stops(py::array_t<T> stops_) {
        makeNative(stops_);
        py::buffer_info stops_info = stops_.request();
        if (stops_info.ndim < 1) {
            throw std::domain_error("stops must have at least 1 dimension");
        }
        int N = stops_info.strides[0] / stops_info.itemsize;
        auto stops_ptr = (T*)stops_info.ptr;
        for (ssize_t i = 0; i < stops_info.size; i++) {
            if (stops_ptr[i * N] < 0) {
                throw std::invalid_argument("stops must have all non-negative values: see index [" + std::to_string(i * N) + "]");
            }
        }
        stops = (py::array_t<std::int64_t>)stops_;
    }
    
    template <typename S>
    JaggedArraySrc(py::array_t<S> starts_, py::array_t<S> stops_, py::array content_) {
        set_starts(starts_);
        set_stops(stops_);
        content_array = content_;
        content_type  = 'a';
    }

    template <typename S>
    JaggedArraySrc(py::array_t<S> starts_, py::array_t<S> stops_, JaggedArraySrc* content_) {
        set_starts(starts_);
        set_stops(stops_);
        content_jagged = content_;
        content_type   = 'j';
    }

    template <typename T>
    static py::array_t<std::int64_t> offsets2parents(py::array_t<T> offsets) {
        makeNative(offsets);
        py::buffer_info offsets_info = offsets.request();
        if (offsets_info.size <= 0) {
            throw std::invalid_argument("offsets must have at least one element");
        }
        auto offsets_ptr = (T*)offsets_info.ptr;
        int N = offsets_info.strides[0] / offsets_info.itemsize;

        ssize_t parents_length = (ssize_t)offsets_ptr[offsets_info.size - 1];
        auto parents = py::array_t<std::int64_t>(parents_length);
        py::buffer_info parents_info = parents.request();

        auto parents_ptr = (std::int64_t*)parents_info.ptr;

        ssize_t j = 0;
        ssize_t k = -1;
        for (ssize_t i = 0; i < offsets_info.size; i++) {
            while (j < (ssize_t)offsets_ptr[i * N]) {
                parents_ptr[j] = (std::int64_t)k;
                j += 1;
            }
            k += 1;
        }

        return parents;
    }

    template <typename T>
    static py::array_t<std::int64_t> counts2offsets(py::array_t<T> counts) {
        makeNative(counts);
        py::buffer_info counts_info = counts.request();
        auto counts_ptr = (T*)counts_info.ptr;
        int N = counts_info.strides[0] / counts_info.itemsize;

        ssize_t offsets_length = counts_info.size + 1;
        auto offsets = py::array_t<std::int64_t>(offsets_length);
        py::buffer_info offsets_info = offsets.request();
        auto offsets_ptr = (std::int64_t*)offsets_info.ptr;

        offsets_ptr[0] = 0;
        for (ssize_t i = 0; i < counts_info.size; i++) {
            offsets_ptr[i + 1] = offsets_ptr[i] + (std::int64_t)counts_ptr[i * N];
        }
        return offsets;
    }

    template <typename T>
    static py::array_t<std::int64_t> startsstops2parents(py::array_t<T> starts, py::array_t<T> stops) {
        makeNative(starts);
        makeNative(stops);
        py::buffer_info starts_info = starts.request();
        auto starts_ptr = (T*)starts_info.ptr;
        int N_starts = starts_info.strides[0] / starts_info.itemsize;

        py::buffer_info stops_info = stops.request();
        auto stops_ptr = (T*)stops_info.ptr;
        int N_stops = stops_info.strides[0] / stops_info.itemsize;

        ssize_t max;
        if (stops_info.size < 1) {
            max = 0;
        }
        else {
            max = (ssize_t)stops_ptr[0];
            for (ssize_t i = 1; i < stops_info.size; i++) {
                if ((ssize_t)stops_ptr[i * N_stops] > max) {
                    max = (ssize_t)stops_ptr[i * N_stops];
                }
            }
        }
        auto parents = py::array_t<std::int64_t>(max);
        py::buffer_info parents_info = parents.request();
        auto parents_ptr = (std::int64_t*)parents_info.ptr;
        for (ssize_t i = 0; i < max; i++) {
            parents_ptr[i] = -1;
        }

        for (ssize_t i = 0; i < starts_info.size; i++) {
            for (ssize_t j = (ssize_t)starts_ptr[i * N_starts]; j < (ssize_t)stops_ptr[i * N_stops]; j++) {
                parents_ptr[j] = (std::int64_t)i;
            }
        }

        return parents;
    }

    template <typename T>
    static py::tuple parents2startsstops(py::array_t<T> parents, T length = -1) {
        makeNative(parents);
        py::buffer_info parents_info = parents.request();
        auto parents_ptr = (T*)parents_info.ptr;
        int N = parents_info.strides[0] / parents_info.itemsize;

        if (length < 0) {
            length = 0;
            for (ssize_t i = 0; i < parents_info.size; i++) {
                if (parents_ptr[i] > length) {
                    length = parents_ptr[i * N];
                }
            }
            length++;
        }

        auto starts = py::array_t<std::int64_t>((ssize_t)length);
        py::buffer_info starts_info = starts.request();
        auto starts_ptr = (std::int64_t*)starts_info.ptr;

        auto stops = py::array_t<std::int64_t>((ssize_t)length);
        py::buffer_info stops_info = stops.request();
        auto stops_ptr = (std::int64_t*)stops_info.ptr;

        for (ssize_t i = 0; i < (ssize_t)length; i++) {
            starts_ptr[i] = 0;
            stops_ptr[i] = 0;
        }

        T last = -1;
        for (ssize_t k = 0; k < parents_info.size; k++) {
            auto thisOne = parents_ptr[k * N];
            if (last != thisOne) {
                if (last >= 0 && last < length) {
                    stops_ptr[last] = (std::int64_t)k;
                }
                if (thisOne >= 0 && thisOne < length) {
                    starts_ptr[thisOne] = (std::int64_t)k;
                }
            }
            last = thisOne;
        }

        if (last != -1) {
            stops_ptr[last] = (std::int64_t)parents_info.size;
        }

        py::list temp;
        temp.append(starts);
        temp.append(stops);
        py::tuple out(temp);
        return out;
    }

    template <typename T>
    static py::tuple uniques2offsetsparents(py::array_t<T> uniques) {
        makeNative(uniques);
        py::buffer_info uniques_info = uniques.request();
        auto uniques_ptr = (T*)uniques_info.ptr;
        int N = uniques_info.strides[0] / uniques_info.itemsize;

        ssize_t tempLength;
        if (uniques_info.size < 1) {
            tempLength = 0;
        }
        else {
            tempLength = uniques_info.size - 1;
        }
        auto tempArray = py::array_t<bool>(tempLength);
        py::buffer_info tempArray_info = tempArray.request();
        auto tempArray_ptr = (bool*)tempArray_info.ptr;

        ssize_t countLength = 0;
        for (ssize_t i = 0; i < uniques_info.size - 1; i++) {
            if (uniques_ptr[i * N] != uniques_ptr[(i + 1) * N]) {
                tempArray_ptr[i] = true;
                countLength++;
            }
            else {
                tempArray_ptr[i] = false;
            }
        }
        auto changes = py::array_t<std::int64_t>(countLength);
        py::buffer_info changes_info = changes.request();
        auto changes_ptr = (std::int64_t*)changes_info.ptr;
        ssize_t index = 0;
        for (ssize_t i = 0; i < tempArray_info.size; i++) {
            if (tempArray_ptr[i]) {
                changes_ptr[index++] = (std::int64_t)(i + 1);
            }
        }

        auto offsets = py::array_t<std::int64_t>(changes_info.size + 2);
        py::buffer_info offsets_info = offsets.request();
        auto offsets_ptr = (std::int64_t*)offsets_info.ptr;
        offsets_ptr[0] = 0;
        offsets_ptr[offsets_info.size - 1] = (std::int64_t)uniques_info.size;
        for (ssize_t i = 1; i < offsets_info.size - 1; i++) {
            offsets_ptr[i] = changes_ptr[i - 1];
        }

        auto parents = py::array_t<std::int64_t>(uniques_info.size);
        py::buffer_info parents_info = parents.request();
        auto parents_ptr = (std::int64_t*)parents_info.ptr;
        for (ssize_t i = 0; i < parents_info.size; i++) {
            parents_ptr[i] = 0;
        }
        for (ssize_t i = 0; i < changes_info.size; i++) {
            parents_ptr[(ssize_t)changes_ptr[i]] = 1;
        }
        for (ssize_t i = 1; i < parents_info.size; i++) {
            parents_ptr[i] += parents_ptr[i - 1];
        }

        py::list temp;
        temp.append(offsets);
        temp.append(parents);
        py::tuple out(temp);
        return out;
    }

    template <typename T>
    py::array_t<std::int64_t> __getitem__(T index) { // limited to single-int arguments, with 1d non-stride content array
        py::buffer_info starts_info = starts.request();
        py::buffer_info stops_info = stops.request();
        if (index < 0) {
            index = starts_info.size + index;
        }
        if (starts_info.size > stops_info.size) {
            throw std::out_of_range("starts must have the same or shorter length than stops");
        }
        if ((ssize_t)index > starts_info.size || index < 0) {
            throw std::out_of_range("index must specify a location within the JaggedArray");
        }
        if (starts_info.ndim != stops_info.ndim) {
            throw std::domain_error("starts and stops must have the same dimensionality");
        }
        ssize_t start = (ssize_t)((std::int64_t*)starts_info.ptr)[index];
        ssize_t stop = (ssize_t)((std::int64_t*)stops_info.ptr)[index];
        if (content_type == 'a') {
            py::buffer_info content_info = content_array.request();
            auto content_ptr = (std::int64_t*)content_info.ptr;
            if (content_info.ndim == 1 && content_info.strides[0] == 8) {
                if (start >= content_info.size || start < 0 || stop > content_info.size || stop < 0) {
                    throw std::out_of_range("starts and stops are not within the bounds of content");
                }
                if (stop >= start) {
                    auto out = py::array_t<std::int64_t>(stop - start);
                    py::buffer_info out_info = out.request();
                    auto out_ptr = (std::int64_t*)out_info.ptr;

                    ssize_t here = 0;
                    for (ssize_t i = start; i < stop; i++) {
                        out_ptr[here++] = content_ptr[i];
                    }
                    return out;
                }
                throw std::out_of_range("stops must be greater than or equal to starts");
            }
        }
        auto out = py::array_t<std::int64_t>(0);
        return out;
    }

    std::string __str__() {
        std::string out = "";

        py::buffer_info starts_info = starts.request();
        auto starts_ptr = (std::int64_t*)starts_info.ptr;

        py::buffer_info stops_info = stops.request();
        auto stops_ptr = (std::int64_t*)stops_info.ptr;

        if (content_type == 'a') {
            py::buffer_info array_info = content_array.request();
            auto array_ptr = (std::int64_t*)array_info.ptr;

            if (array_info.ndim == 1 && array_info.strides[0] / array_info.itemsize == 1) {
                if (starts_info.size > stops_info.size) {
                    throw std::out_of_range("starts must be the same or shorter length than stops");
                }

                ssize_t limit = starts_info.size;
                for (ssize_t i = 0; i < limit; i++) {
                    if (i != 0) {
                        out = out + " ";
                    }
                    out = out + "[";

                    ssize_t end = (ssize_t)stops_ptr[i];
                    if ((ssize_t)starts_ptr[i] > end) {
                        throw std::out_of_range("stops must be greater than or equal to starts");
                    }

                    for (ssize_t j = (ssize_t)starts_ptr[i]; j < end; j++) {
                        if (j != (ssize_t)starts_ptr[i]) {
                            out = out + " ";
                        }
                        out = out + std::to_string(array_ptr[j]);
                    }
                    out = out + "]";
                }
                return "[" + out + "]";
            }
        }
        return "-Error: print function is not yet implemented for this type-";
    }

    std::string __repr__() { // limited functionality
        return "<JaggedArray " + __str__() + ">";
    }

    ssize_t __len__() {
        return starts.request().size;
    }

    JaggedArraySrc* __iter__() {
        iter_index = 0;
        return this;
    }

    py::array_t<int64_t> __next__() { // very limited, like getitem and repr
        if (iter_index >= starts.request().size) {
            throw py::stop_iteration();
        }
        return __getitem__(iter_index++);
    }
};

#define DEF(deftype, METHOD) .deftype(#METHOD, &JaggedArraySrc::METHOD<std::int64_t>)\
.deftype(#METHOD, &JaggedArraySrc::METHOD<std::uint64_t>)\
.deftype(#METHOD, &JaggedArraySrc::METHOD<std::int32_t>)\
.deftype(#METHOD, &JaggedArraySrc::METHOD<std::uint32_t>)\
.deftype(#METHOD, &JaggedArraySrc::METHOD<std::int16_t>)\
.deftype(#METHOD, &JaggedArraySrc::METHOD<std::uint16_t>)\
.deftype(#METHOD, &JaggedArraySrc::METHOD<std::int8_t>)\
.deftype(#METHOD, &JaggedArraySrc::METHOD<std::uint8_t>)

#define INIT(TYPE) .def(py::init<py::array_t<std::int8_t>, py::array_t<std::int8_t>, TYPE>())\
.def(py::init<py::array_t<std::uint8_t>, py::array_t<std::uint8_t>, TYPE>())\
.def(py::init<py::array_t<std::int16_t>, py::array_t<std::int16_t>, TYPE>())\
.def(py::init<py::array_t<std::uint16_t>, py::array_t<std::uint16_t>, TYPE>())\
.def(py::init<py::array_t<std::int32_t>, py::array_t<std::int32_t>, TYPE>())\
.def(py::init<py::array_t<std::uint32_t>, py::array_t<std::uint32_t>, TYPE>())\
.def(py::init<py::array_t<std::int64_t>, py::array_t<std::int64_t>, TYPE>())\
.def(py::init<py::array_t<std::uint64_t>, py::array_t<std::uint64_t>, TYPE>())

#define PROPERTY(NAME, GET, SET) .def_property(#NAME, &JaggedArraySrc::GET, &JaggedArraySrc::SET<std::int8_t>)\
.def_property(#NAME, &JaggedArraySrc::GET, &JaggedArraySrc::SET<std::uint8_t>)\
.def_property(#NAME, &JaggedArraySrc::GET, &JaggedArraySrc::SET<std::int16_t>)\
.def_property(#NAME, &JaggedArraySrc::GET, &JaggedArraySrc::SET<std::uint16_t>)\
.def_property(#NAME, &JaggedArraySrc::GET, &JaggedArraySrc::SET<std::int32_t>)\
.def_property(#NAME, &JaggedArraySrc::GET, &JaggedArraySrc::SET<std::uint32_t>)\
.def_property(#NAME, &JaggedArraySrc::GET, &JaggedArraySrc::SET<std::int64_t>)\
.def_property(#NAME, &JaggedArraySrc::GET, &JaggedArraySrc::SET<std::uint64_t>)

PYBIND11_MODULE(_jagged, m) {
    py::class_<JaggedArraySrc>(m, "JaggedArraySrc")
        DEF(def_static, offsets2parents)
        DEF(def_static, counts2offsets)
        DEF(def_static, startsstops2parents)
        DEF(def_static, parents2startsstops)
        DEF(def_static, uniques2offsetsparents)
        INIT(py::array)
        INIT(JaggedArraySrc*)
        .def_property_readonly("content_type", &JaggedArraySrc::get_content_type)
        PROPERTY(starts, get_starts, set_starts)
        PROPERTY(stops, get_stops, set_stops)
        .def_property("content_array", &JaggedArraySrc::get_content_array, &JaggedArraySrc::set_content_array)
        .def_property("content_jagged", &JaggedArraySrc::get_content_jagged, &JaggedArraySrc::set_content_jagged)
        DEF(def, __getitem__)
        .def("__str__", &JaggedArraySrc::__str__)
        .def("__repr__", &JaggedArraySrc::__repr__)
        .def("__len__", &JaggedArraySrc::__len__)
        .def("__iter__", &JaggedArraySrc::__iter__)
        .def("__next__", &JaggedArraySrc::__next__);
}
