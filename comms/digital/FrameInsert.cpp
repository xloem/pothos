// Copyright (c) 2015-2015 Josh Blum
// SPDX-License-Identifier: BSL-1.0

#include "FrameHelper.hpp"
#include <Pothos/Framework.hpp>
#include <cstring> //memcpy
#include <iostream>
#include <algorithm> //min/max
#include <complex>
#include <cstdint>

/***********************************************************************
 * |PothosDoc Frame Insert
 *
 * The Frame Insert block inserts a PHY frame synchronization header.
 * The Frame Sync companion block in the receiver uses this header
 * to locate frames and to synchronize frequency and phase offsets.
 * The Frame Insert block uses start and end label IDs
 * to locate packet boundaries in a stream of samples.
 *
 * <h2>Label adjustment</h2>
 *
 * All labels with the same index as the frame start label
 * (and including the frame start label itself),
 * will be shifted to the start of the frame header.
 * All other labels propagate with the same position.
 *
 * All labels with the same index as the frame end label
 * (and including the frame end label itself),
 * will be shifted to the last symbol of the padding buffer.
 * All other labels propagate with the same position.
 *
 * |category /Digital
 * |keywords preamble frame sync
 * |alias /blocks/frame_insert
 *
 * |param dtype[Data Type] The input data type consumed by the slicer.
 * |widget DTypeChooser(cfloat=1)
 * |default "complex_float64"
 * |preview disable
 *
 * |param preamble A vector of symbols representing the preamble.
 * |default [1, 1, -1]
 * |option [Barker Code 2] \[1, -1\]
 * |option [Barker Code 3] \[1, 1, -1\]
 * |option [Barker Code 4] \[1, 1, -1, 1\]
 * |option [Barker Code 5] \[1, 1, 1, -1, 1\]
 *
 * |param headerId [Header ID] a unique 8-bit ID that will be encoded into the frame header.
 * The frame sync at the receiver uses this ID to reject unrecognized frames.
 * |default 0x55
 *
 * |param symbolWidth [Symbol Width] The number of samples per preamble symbol.
 * Each symbol in the preamble will be duplicated by the specified symbol width.
 * Note: this is not the same as the samples per symbol used in data modulation,
 * and is is typically many times greater for synchronization purposes.
 * |default 20
 * |units samples
 *
 * |param frameStartId[Frame Start ID] The label ID that marks the first symbol of frame data.
 * |default "frameStart"
 * |widget StringEntry()
 *
 * |param frameEndId[Frame End ID] The label ID that marks the last symbol of frame data.
 * |default "frameEnd"
 * |widget StringEntry()
 *
 * |param paddingSize[Padding Size] The number of symbols to pad at the end of a frame.
 * This parameter controls the length of the padding and can be zero to disable it.
 * |default 0
 * |preview valid
 *
 * |factory /comms/frame_insert(dtype)
 * |setter setPreamble(preamble)
 * |setter setHeaderId(headerId)
 * |setter setSymbolWidth(symbolWidth)
 * |setter setFrameStartId(frameStartId)
 * |setter setFrameEndId(frameEndId)
 * |setter setPaddingSize(paddingSize)
 **********************************************************************/
template <typename Type>
class FrameInsert : public Pothos::Block
{
public:
    static Block *make(void)
    {
        return new FrameInsert();
    }

    FrameInsert(void):
        _headerId(0),
        _symbolWidth(0),
        _syncWordWidth(0)
    {
        this->setupInput(0, typeid(Type));
        this->setupOutput(0, typeid(Type), this->uid()); //unique domain because of buffer forwarding
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, setPreamble));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, getPreamble));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, setHeaderId));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, getHeaderId));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, setSymbolWidth));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, getSymbolWidth));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, setFrameStartId));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, getFrameStartId));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, setFrameEndId));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, getFrameEndId));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, setPaddingSize));
        this->registerCall(this, POTHOS_FCN_TUPLE(FrameInsert, getPaddingSize));

        this->setHeaderId(0x55); //initial update
        this->setSymbolWidth(20); //initial update
        this->setPreamble(std::vector<Type>(1, 1)); //initial update
        this->setFrameStartId("frameStart"); //initial update
        this->setFrameEndId("frameEnd"); //initial update
    }

    void setPreamble(const std::vector<Type> preamble)
    {
        if (preamble.empty()) throw Pothos::InvalidArgumentException("FrameInsert::setPreamble()", "preamble cannot be empty");
        _preamble = preamble;
        this->updatePreambleBuffer();
    }

    std::vector<Type> getPreamble(void) const
    {
        return _preamble;
    }

    void setHeaderId(const unsigned char id)
    {
        _headerId = id;
    }

    unsigned char getHeaderId(void) const
    {
        return _headerId;
    }

    void setSymbolWidth(const size_t width)
    {
        if (width == 0) throw Pothos::InvalidArgumentException("FrameInsert::setSymbolWidth()", "symbol width cannot be 0");
        _symbolWidth = width;
        this->updatePreambleBuffer();
    }

    size_t getSymbolWidth(void) const
    {
        return _symbolWidth;
    }

    void setFrameStartId(std::string id)
    {
        _frameStartId = id;
    }

    std::string getFrameStartId(void) const
    {
        return _frameStartId;
    }

    void setFrameEndId(std::string id)
    {
        _frameEndId = id;
    }

    std::string getFrameEndId(void) const
    {
        return _frameEndId;
    }

    void setPaddingSize(const size_t size)
    {
        _paddingBuff = Pothos::BufferChunk(typeid(Type), size);
        std::memset(_paddingBuff.as<void *>(), 0, _paddingBuff.length);
    }

    size_t getPaddingSize(void) const
    {
        return _paddingBuff.elements();
    }

    void work(void)
    {
        auto inputPort = this->input(0);
        auto outputPort = this->output(0);
        size_t consumed = 0;

        //get input buffer
        auto inBuff = inputPort->buffer();
        if (inBuff.length == 0) return;

        //label propagation offset incremented as preambles are posted
        size_t labelIndexOffset = 0;

        //track the index of the last found frame start label
        int lastFoundIndex = -1;

        for (auto &label : inputPort->labels())
        {
            // Skip any label that doesn't yet appear in the data buffer
            if (label.index >= inputPort->elements()) continue;

            Pothos::Label outLabel(label); //modified and posted below

            //increment the offset as soon as we are past the last found index
            if (lastFoundIndex != -1 and size_t(lastFoundIndex) != label.index)
            {
                lastFoundIndex = -1;
                labelIndexOffset += _preambleBuff.elements();
            }

            if (label.id == _frameStartId)
            {
                //post everything before this label
                Pothos::BufferChunk headBuff = inBuff;
                size_t headElems = label.index - consumed;
                headBuff.length = headElems*sizeof(Type);
                if (headBuff.length != 0) outputPort->postBuffer(headBuff);

                //fill the preamble buffer
                Pothos::BufferChunk newPreambleBuff(typeid(Type), _preambleBuff.elements());
                std::memcpy(newPreambleBuff.as<void *>(), _preambleBuff.as<const void *>(), _preambleBuff.length);
                auto p = newPreambleBuff.as<Type *>() + _syncWordWidth;

                //encode the header field into bits
                char headerBits[NUM_HEADER_BITS];
                FrameHeaderFields headerFields;
                headerFields.id = _headerId;
                headerFields.length = 0;
                if (label.data.canConvert(typeid(size_t)))
                {
                    headerFields.length = label.data.template convert<size_t>()*label.width;
                }
                headerFields.chksum = headerFields.doChecksum();
                encodeHeaderWord(headerBits, headerFields);

                //encode header fields as BPSK into the preamble buffer
                const auto sym = _preamble.back();
                for (size_t i = 0; i < NUM_HEADER_BITS; i++)
                {
                    *p++ = (headerBits[i] != 0)?+sym:-sym;
                }

                //post the preamble buffer
                outputPort->postBuffer(newPreambleBuff);

                //remove header from the remaining buffer
                inBuff.length -= headBuff.length;
                inBuff.address += headBuff.length;
                consumed += headBuff.elements();

                //mark for increment on next label index
                lastFoundIndex = label.index;
            }

            else if (label.id == _frameEndId)
            {
                //post everything before this label
                Pothos::BufferChunk headBuff = inBuff;
                size_t headElems = label.index + label.width - consumed; //place at end of width
                headBuff.length = headElems*sizeof(Type);
                headBuff.length = std::min(headBuff.length, inBuff.length); //bounds check
                if (headBuff.length != 0) outputPort->postBuffer(headBuff);

                //post the buffer
                outputPort->postBuffer(_paddingBuff);

                //remove header from the remaining buffer
                inBuff.length -= headBuff.length;
                inBuff.address += headBuff.length;
                consumed += headBuff.elements();

                //increment index for insertion of padding
                labelIndexOffset += _paddingBuff.elements();
            }

            //propagate labels here with the offset
            outLabel.index += labelIndexOffset;
            outputPort->postLabel(outLabel);
        }

        //post the remaining bytes
        if (inBuff.length != 0) outputPort->postBuffer(inBuff);

        //consume the entire buffer
        inputPort->consume(inputPort->elements());
    }

    void propagateLabels(const Pothos::InputPort *)
    {
        //don't propagate here, its done in work()
    }

private:

    void updatePreambleBuffer(void)
    {
        _syncWordWidth = _symbolWidth*_preamble.size();
        _preambleBuff = Pothos::BufferChunk(typeid(Type), _syncWordWidth+NUM_HEADER_BITS);

        auto p = _preambleBuff.as<Type *>();
        std::memset(p, 0, _preambleBuff.length);
        for (size_t i = 0; i < _preamble.size(); i++)
        {
            for (size_t j = 0; j < _symbolWidth; j++)
            {
                *p++ = _preamble[i];
            }
        }
    }

    std::string _frameStartId;
    std::string _frameEndId;
    std::vector<Type> _preamble;
    unsigned char _headerId;
    size_t _symbolWidth;
    size_t _syncWordWidth;
    Pothos::BufferChunk _preambleBuff;
    Pothos::BufferChunk _paddingBuff;
};

/***********************************************************************
 * registration
 **********************************************************************/
static Pothos::Block *FrameInsertFactory(const Pothos::DType &dtype)
{
    #define ifTypeDeclareFactory(type) \
        if (dtype == Pothos::DType(typeid(std::complex<type>))) \
            return new FrameInsert<std::complex<type>>();
    ifTypeDeclareFactory(double);
    ifTypeDeclareFactory(float);
    throw Pothos::InvalidArgumentException("FrameInsertFactory("+dtype.toString()+")", "unsupported type");
}

static Pothos::BlockRegistry registerFrameInsert(
    "/comms/frame_insert", &FrameInsertFactory);

static Pothos::BlockRegistry registerFrameInsertOldPath(
    "/blocks/frame_insert", &FrameInsertFactory);
