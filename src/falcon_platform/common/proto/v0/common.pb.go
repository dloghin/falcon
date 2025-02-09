// Code generated by protoc-gen-go. DO NOT EDIT.
// versions:
// 	protoc-gen-go v1.28.0
// 	protoc        v3.19.4
// source: common.proto

package v0

import (
	protoreflect "google.golang.org/protobuf/reflect/protoreflect"
	protoimpl "google.golang.org/protobuf/runtime/protoimpl"
	reflect "reflect"
	sync "sync"
)

const (
	// Verify that this generated code is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(20 - protoimpl.MinVersion)
	// Verify that runtime/protoimpl is sufficiently up-to-date.
	_ = protoimpl.EnforceVersion(protoimpl.MaxVersion - 20)
)

// This message is used when initializing the training data
// and testing data, as well as transmitting batch ids in each training iteration
type IntArray struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	IntItem []int32 `protobuf:"varint,1,rep,packed,name=int_item,json=intItem,proto3" json:"int_item,omitempty"`
}

func (x *IntArray) Reset() {
	*x = IntArray{}
	if protoimpl.UnsafeEnabled {
		mi := &file_common_proto_msgTypes[0]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *IntArray) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*IntArray) ProtoMessage() {}

func (x *IntArray) ProtoReflect() protoreflect.Message {
	mi := &file_common_proto_msgTypes[0]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use IntArray.ProtoReflect.Descriptor instead.
func (*IntArray) Descriptor() ([]byte, []int) {
	return file_common_proto_rawDescGZIP(), []int{0}
}

func (x *IntArray) GetIntItem() []int32 {
	if x != nil {
		return x.IntItem
	}
	return nil
}

// This message is used when transmitting the training labels and testing labels
type DoubleArray struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Item []float64 `protobuf:"fixed64,1,rep,packed,name=item,proto3" json:"item,omitempty"`
}

func (x *DoubleArray) Reset() {
	*x = DoubleArray{}
	if protoimpl.UnsafeEnabled {
		mi := &file_common_proto_msgTypes[1]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *DoubleArray) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*DoubleArray) ProtoMessage() {}

func (x *DoubleArray) ProtoReflect() protoreflect.Message {
	mi := &file_common_proto_msgTypes[1]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use DoubleArray.ProtoReflect.Descriptor instead.
func (*DoubleArray) Descriptor() ([]byte, []int) {
	return file_common_proto_rawDescGZIP(), []int{1}
}

func (x *DoubleArray) GetItem() []float64 {
	if x != nil {
		return x.Item
	}
	return nil
}

// This message is used when transmitting the training data and testing data
type DoubleMatrix struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	Array []*DoubleArray `protobuf:"bytes,1,rep,name=array,proto3" json:"array,omitempty"`
}

func (x *DoubleMatrix) Reset() {
	*x = DoubleMatrix{}
	if protoimpl.UnsafeEnabled {
		mi := &file_common_proto_msgTypes[2]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *DoubleMatrix) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*DoubleMatrix) ProtoMessage() {}

func (x *DoubleMatrix) ProtoReflect() protoreflect.Message {
	mi := &file_common_proto_msgTypes[2]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use DoubleMatrix.ProtoReflect.Descriptor instead.
func (*DoubleMatrix) Descriptor() ([]byte, []int) {
	return file_common_proto_rawDescGZIP(), []int{2}
}

func (x *DoubleMatrix) GetArray() []*DoubleArray {
	if x != nil {
		return x.Array
	}
	return nil
}

// This message is used to serialize an EncodedNumber value
type FixedPointEncodedNumber struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	// maximum value
	N string `protobuf:"bytes,1,opt,name=n,proto3" json:"n,omitempty"`
	// encoded value
	Value string `protobuf:"bytes,2,opt,name=value,proto3" json:"value,omitempty"`
	// fixed point precision
	Exponent int32 `protobuf:"varint,3,opt,name=exponent,proto3" json:"exponent,omitempty"`
	// value type
	Type int32 `protobuf:"varint,4,opt,name=type,proto3" json:"type,omitempty"`
}

func (x *FixedPointEncodedNumber) Reset() {
	*x = FixedPointEncodedNumber{}
	if protoimpl.UnsafeEnabled {
		mi := &file_common_proto_msgTypes[3]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *FixedPointEncodedNumber) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*FixedPointEncodedNumber) ProtoMessage() {}

func (x *FixedPointEncodedNumber) ProtoReflect() protoreflect.Message {
	mi := &file_common_proto_msgTypes[3]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use FixedPointEncodedNumber.ProtoReflect.Descriptor instead.
func (*FixedPointEncodedNumber) Descriptor() ([]byte, []int) {
	return file_common_proto_rawDescGZIP(), []int{3}
}

func (x *FixedPointEncodedNumber) GetN() string {
	if x != nil {
		return x.N
	}
	return ""
}

func (x *FixedPointEncodedNumber) GetValue() string {
	if x != nil {
		return x.Value
	}
	return ""
}

func (x *FixedPointEncodedNumber) GetExponent() int32 {
	if x != nil {
		return x.Exponent
	}
	return 0
}

func (x *FixedPointEncodedNumber) GetType() int32 {
	if x != nil {
		return x.Type
	}
	return 0
}

type EncodedNumberArray struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	EncodedNumber []*FixedPointEncodedNumber `protobuf:"bytes,1,rep,name=encoded_number,json=encodedNumber,proto3" json:"encoded_number,omitempty"`
}

func (x *EncodedNumberArray) Reset() {
	*x = EncodedNumberArray{}
	if protoimpl.UnsafeEnabled {
		mi := &file_common_proto_msgTypes[4]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *EncodedNumberArray) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*EncodedNumberArray) ProtoMessage() {}

func (x *EncodedNumberArray) ProtoReflect() protoreflect.Message {
	mi := &file_common_proto_msgTypes[4]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use EncodedNumberArray.ProtoReflect.Descriptor instead.
func (*EncodedNumberArray) Descriptor() ([]byte, []int) {
	return file_common_proto_rawDescGZIP(), []int{4}
}

func (x *EncodedNumberArray) GetEncodedNumber() []*FixedPointEncodedNumber {
	if x != nil {
		return x.EncodedNumber
	}
	return nil
}

type EncodedNumberMatrix struct {
	state         protoimpl.MessageState
	sizeCache     protoimpl.SizeCache
	unknownFields protoimpl.UnknownFields

	EncodedArray []*EncodedNumberArray `protobuf:"bytes,1,rep,name=encoded_array,json=encodedArray,proto3" json:"encoded_array,omitempty"`
}

func (x *EncodedNumberMatrix) Reset() {
	*x = EncodedNumberMatrix{}
	if protoimpl.UnsafeEnabled {
		mi := &file_common_proto_msgTypes[5]
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		ms.StoreMessageInfo(mi)
	}
}

func (x *EncodedNumberMatrix) String() string {
	return protoimpl.X.MessageStringOf(x)
}

func (*EncodedNumberMatrix) ProtoMessage() {}

func (x *EncodedNumberMatrix) ProtoReflect() protoreflect.Message {
	mi := &file_common_proto_msgTypes[5]
	if protoimpl.UnsafeEnabled && x != nil {
		ms := protoimpl.X.MessageStateOf(protoimpl.Pointer(x))
		if ms.LoadMessageInfo() == nil {
			ms.StoreMessageInfo(mi)
		}
		return ms
	}
	return mi.MessageOf(x)
}

// Deprecated: Use EncodedNumberMatrix.ProtoReflect.Descriptor instead.
func (*EncodedNumberMatrix) Descriptor() ([]byte, []int) {
	return file_common_proto_rawDescGZIP(), []int{5}
}

func (x *EncodedNumberMatrix) GetEncodedArray() []*EncodedNumberArray {
	if x != nil {
		return x.EncodedArray
	}
	return nil
}

var File_common_proto protoreflect.FileDescriptor

var file_common_proto_rawDesc = []byte{
	0x0a, 0x0c, 0x63, 0x6f, 0x6d, 0x6d, 0x6f, 0x6e, 0x2e, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x12, 0x19,
	0x63, 0x6f, 0x6d, 0x2e, 0x6e, 0x75, 0x73, 0x2e, 0x64, 0x62, 0x73, 0x79, 0x74, 0x65, 0x6d, 0x2e,
	0x66, 0x61, 0x6c, 0x63, 0x6f, 0x6e, 0x2e, 0x76, 0x30, 0x22, 0x25, 0x0a, 0x08, 0x49, 0x6e, 0x74,
	0x41, 0x72, 0x72, 0x61, 0x79, 0x12, 0x19, 0x0a, 0x08, 0x69, 0x6e, 0x74, 0x5f, 0x69, 0x74, 0x65,
	0x6d, 0x18, 0x01, 0x20, 0x03, 0x28, 0x05, 0x52, 0x07, 0x69, 0x6e, 0x74, 0x49, 0x74, 0x65, 0x6d,
	0x22, 0x21, 0x0a, 0x0b, 0x44, 0x6f, 0x75, 0x62, 0x6c, 0x65, 0x41, 0x72, 0x72, 0x61, 0x79, 0x12,
	0x12, 0x0a, 0x04, 0x69, 0x74, 0x65, 0x6d, 0x18, 0x01, 0x20, 0x03, 0x28, 0x01, 0x52, 0x04, 0x69,
	0x74, 0x65, 0x6d, 0x22, 0x4c, 0x0a, 0x0c, 0x44, 0x6f, 0x75, 0x62, 0x6c, 0x65, 0x4d, 0x61, 0x74,
	0x72, 0x69, 0x78, 0x12, 0x3c, 0x0a, 0x05, 0x61, 0x72, 0x72, 0x61, 0x79, 0x18, 0x01, 0x20, 0x03,
	0x28, 0x0b, 0x32, 0x26, 0x2e, 0x63, 0x6f, 0x6d, 0x2e, 0x6e, 0x75, 0x73, 0x2e, 0x64, 0x62, 0x73,
	0x79, 0x74, 0x65, 0x6d, 0x2e, 0x66, 0x61, 0x6c, 0x63, 0x6f, 0x6e, 0x2e, 0x76, 0x30, 0x2e, 0x44,
	0x6f, 0x75, 0x62, 0x6c, 0x65, 0x41, 0x72, 0x72, 0x61, 0x79, 0x52, 0x05, 0x61, 0x72, 0x72, 0x61,
	0x79, 0x22, 0x6d, 0x0a, 0x17, 0x46, 0x69, 0x78, 0x65, 0x64, 0x50, 0x6f, 0x69, 0x6e, 0x74, 0x45,
	0x6e, 0x63, 0x6f, 0x64, 0x65, 0x64, 0x4e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x12, 0x0c, 0x0a, 0x01,
	0x6e, 0x18, 0x01, 0x20, 0x01, 0x28, 0x09, 0x52, 0x01, 0x6e, 0x12, 0x14, 0x0a, 0x05, 0x76, 0x61,
	0x6c, 0x75, 0x65, 0x18, 0x02, 0x20, 0x01, 0x28, 0x09, 0x52, 0x05, 0x76, 0x61, 0x6c, 0x75, 0x65,
	0x12, 0x1a, 0x0a, 0x08, 0x65, 0x78, 0x70, 0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x18, 0x03, 0x20, 0x01,
	0x28, 0x05, 0x52, 0x08, 0x65, 0x78, 0x70, 0x6f, 0x6e, 0x65, 0x6e, 0x74, 0x12, 0x12, 0x0a, 0x04,
	0x74, 0x79, 0x70, 0x65, 0x18, 0x04, 0x20, 0x01, 0x28, 0x05, 0x52, 0x04, 0x74, 0x79, 0x70, 0x65,
	0x22, 0x6f, 0x0a, 0x12, 0x45, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x64, 0x4e, 0x75, 0x6d, 0x62, 0x65,
	0x72, 0x41, 0x72, 0x72, 0x61, 0x79, 0x12, 0x59, 0x0a, 0x0e, 0x65, 0x6e, 0x63, 0x6f, 0x64, 0x65,
	0x64, 0x5f, 0x6e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x18, 0x01, 0x20, 0x03, 0x28, 0x0b, 0x32, 0x32,
	0x2e, 0x63, 0x6f, 0x6d, 0x2e, 0x6e, 0x75, 0x73, 0x2e, 0x64, 0x62, 0x73, 0x79, 0x74, 0x65, 0x6d,
	0x2e, 0x66, 0x61, 0x6c, 0x63, 0x6f, 0x6e, 0x2e, 0x76, 0x30, 0x2e, 0x46, 0x69, 0x78, 0x65, 0x64,
	0x50, 0x6f, 0x69, 0x6e, 0x74, 0x45, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x64, 0x4e, 0x75, 0x6d, 0x62,
	0x65, 0x72, 0x52, 0x0d, 0x65, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x64, 0x4e, 0x75, 0x6d, 0x62, 0x65,
	0x72, 0x22, 0x69, 0x0a, 0x13, 0x45, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x64, 0x4e, 0x75, 0x6d, 0x62,
	0x65, 0x72, 0x4d, 0x61, 0x74, 0x72, 0x69, 0x78, 0x12, 0x52, 0x0a, 0x0d, 0x65, 0x6e, 0x63, 0x6f,
	0x64, 0x65, 0x64, 0x5f, 0x61, 0x72, 0x72, 0x61, 0x79, 0x18, 0x01, 0x20, 0x03, 0x28, 0x0b, 0x32,
	0x2d, 0x2e, 0x63, 0x6f, 0x6d, 0x2e, 0x6e, 0x75, 0x73, 0x2e, 0x64, 0x62, 0x73, 0x79, 0x74, 0x65,
	0x6d, 0x2e, 0x66, 0x61, 0x6c, 0x63, 0x6f, 0x6e, 0x2e, 0x76, 0x30, 0x2e, 0x45, 0x6e, 0x63, 0x6f,
	0x64, 0x65, 0x64, 0x4e, 0x75, 0x6d, 0x62, 0x65, 0x72, 0x41, 0x72, 0x72, 0x61, 0x79, 0x52, 0x0c,
	0x65, 0x6e, 0x63, 0x6f, 0x64, 0x65, 0x64, 0x41, 0x72, 0x72, 0x61, 0x79, 0x42, 0x05, 0x5a, 0x03,
	0x2f, 0x76, 0x30, 0x62, 0x06, 0x70, 0x72, 0x6f, 0x74, 0x6f, 0x33,
}

var (
	file_common_proto_rawDescOnce sync.Once
	file_common_proto_rawDescData = file_common_proto_rawDesc
)

func file_common_proto_rawDescGZIP() []byte {
	file_common_proto_rawDescOnce.Do(func() {
		file_common_proto_rawDescData = protoimpl.X.CompressGZIP(file_common_proto_rawDescData)
	})
	return file_common_proto_rawDescData
}

var file_common_proto_msgTypes = make([]protoimpl.MessageInfo, 6)
var file_common_proto_goTypes = []interface{}{
	(*IntArray)(nil),                // 0: com.nus.dbsytem.falcon.v0.IntArray
	(*DoubleArray)(nil),             // 1: com.nus.dbsytem.falcon.v0.DoubleArray
	(*DoubleMatrix)(nil),            // 2: com.nus.dbsytem.falcon.v0.DoubleMatrix
	(*FixedPointEncodedNumber)(nil), // 3: com.nus.dbsytem.falcon.v0.FixedPointEncodedNumber
	(*EncodedNumberArray)(nil),      // 4: com.nus.dbsytem.falcon.v0.EncodedNumberArray
	(*EncodedNumberMatrix)(nil),     // 5: com.nus.dbsytem.falcon.v0.EncodedNumberMatrix
}
var file_common_proto_depIdxs = []int32{
	1, // 0: com.nus.dbsytem.falcon.v0.DoubleMatrix.array:type_name -> com.nus.dbsytem.falcon.v0.DoubleArray
	3, // 1: com.nus.dbsytem.falcon.v0.EncodedNumberArray.encoded_number:type_name -> com.nus.dbsytem.falcon.v0.FixedPointEncodedNumber
	4, // 2: com.nus.dbsytem.falcon.v0.EncodedNumberMatrix.encoded_array:type_name -> com.nus.dbsytem.falcon.v0.EncodedNumberArray
	3, // [3:3] is the sub-list for method output_type
	3, // [3:3] is the sub-list for method input_type
	3, // [3:3] is the sub-list for extension type_name
	3, // [3:3] is the sub-list for extension extendee
	0, // [0:3] is the sub-list for field type_name
}

func init() { file_common_proto_init() }
func file_common_proto_init() {
	if File_common_proto != nil {
		return
	}
	if !protoimpl.UnsafeEnabled {
		file_common_proto_msgTypes[0].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*IntArray); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_common_proto_msgTypes[1].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*DoubleArray); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_common_proto_msgTypes[2].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*DoubleMatrix); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_common_proto_msgTypes[3].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*FixedPointEncodedNumber); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_common_proto_msgTypes[4].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*EncodedNumberArray); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
		file_common_proto_msgTypes[5].Exporter = func(v interface{}, i int) interface{} {
			switch v := v.(*EncodedNumberMatrix); i {
			case 0:
				return &v.state
			case 1:
				return &v.sizeCache
			case 2:
				return &v.unknownFields
			default:
				return nil
			}
		}
	}
	type x struct{}
	out := protoimpl.TypeBuilder{
		File: protoimpl.DescBuilder{
			GoPackagePath: reflect.TypeOf(x{}).PkgPath(),
			RawDescriptor: file_common_proto_rawDesc,
			NumEnums:      0,
			NumMessages:   6,
			NumExtensions: 0,
			NumServices:   0,
		},
		GoTypes:           file_common_proto_goTypes,
		DependencyIndexes: file_common_proto_depIdxs,
		MessageInfos:      file_common_proto_msgTypes,
	}.Build()
	File_common_proto = out.File
	file_common_proto_rawDesc = nil
	file_common_proto_goTypes = nil
	file_common_proto_depIdxs = nil
}
