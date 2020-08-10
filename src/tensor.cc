#include "tensor.h"
#include "util.h"

using namespace toC;
void Tensor::parse_onnx_tensor(const onnx::TensorProto &tensor)
{

	generate=true;
	initialize=true;

	// assert tensor is resolvable
	if( onnx::TensorProto_DataLocation() != onnx::TensorProto_DataLocation_DEFAULT )
		ERROR("unhandled: non-default data location in tensor " << tensor.name());
	if( tensor.has_segment() )
		ERROR("unhandled: segmented data in tensor" << tensor.name());


	int32_t datatype = tensor.data_type();
	if( onnx::TensorProto_DataType_IsValid(datatype) == false )
		ERROR("Non-valid data type " << datatype << " in tensor " << tensor.name());
	data_type = static_cast<onnx::TensorProto_DataType>(datatype);

	int64_t data_num_elem; // Can data size be negative? onnx.pb.h encodes size into 'signed int'
	switch( datatype )
	{
		case onnx::TensorProto_DataType_FLOAT:
			data_num_elem = tensor.float_data_size(); break;

		case onnx::TensorProto_DataType_INT32:
			data_num_elem = tensor.int32_data_size(); break;
		case onnx::TensorProto_DataType_INT64:
			data_num_elem = tensor.int64_data_size(); break;
		default:
			ERROR("unhandled tensor data type in tensor " << tensor.name());
			break;
	};

	// Save the data dimensions, and calculate number of elements based on this data.
	// At least ONNX testsuite input & output, dumped as protobuffers, have
	// data_num_elem set to zero in them. TODO: figure out if there is something behind this
	int64_t calc_num_data =1;
	for( int dim : tensor.dims() ) {
		data_dim.push_back(dim);
		calc_num_data *= dim;
	}
	if( data_num_elem != calc_num_data ) {
		if( data_num_elem != 0 )
			ERROR("Error: data size does not match dimensions, and data_num_elem is not zero");
		else if( tensor.has_raw_data() == false )
			ERROR("Error: data size does not match dimensions, and no raw data");
		else
			data_num_elem = calc_num_data;
	}

	data_num_elem = data_num_elem;
	data_buffer = malloc(data_num_elem * data_elem_size());
	if( data_buffer == NULL )
		ERROR("memory allocation failed for tensor " << tensor.name());

	if( tensor.has_raw_data() ) {
		std::string raw_data = tensor.raw_data(); // Yes, std::string!
		if( raw_data.size() != (uint64_t)(calc_num_data*data_elem_size()) )
			ERROR("Error: tensor raw data size does not match dimensions");

		memcpy( data_buffer, raw_data.c_str(), raw_data.size() );
	}

	else {
		switch( datatype )
		{
			case onnx::TensorProto_DataType_FLOAT:
				for( int i=0; i<data_num_elem; i++  )
					((float*)data_buffer)[i] = tensor.float_data(i);
				break;
			case onnx::TensorProto_DataType_INT32:
				for( int i=0; i<data_num_elem; i++  )
					((int32_t*)data_buffer)[i] = tensor.int32_data(i);
				break;
			case onnx::TensorProto_DataType_INT64:
				for( int i=0; i<data_num_elem; i++  )
					((int64_t*)data_buffer)[i] = tensor.int64_data(i);
				break;
			default:
				ERROR("unhandled tensor data type in tensor " << tensor.name());
				break;
		};
	}

	name = tensor.name();
	doc = tensor.doc_string();

}

std::string Tensor::cname(void) const
{
	return "tensor_" + cify_name(name);
}

int Tensor::data_elem_size(void)const
{
	switch( data_type )
	{
	case onnx::TensorProto_DataType_FLOAT:
			return sizeof(float); break;
		case onnx::TensorProto_DataType_INT32:
			return sizeof(int32_t); break;
		case onnx::TensorProto_DataType_INT64:
			return sizeof(int64_t); break;
		default:
			ERROR("unhandled tensor data type in tensor " << name);
			break;
	};
}

std::string Tensor::data_type_str(void) const
{
	switch( data_type )
	{
		case onnx::TensorProto_DataType_FLOAT:
			return "float"; break;
		case onnx::TensorProto_DataType_INT32:
			return "int32_t"; break;
		case onnx::TensorProto_DataType_INT64:
			return "int64_t"; break;
		default:
			ERROR("unhandled tensor data type in tensor " << name);
			break;
	};
}


/* Print the tensor initializer.
 * Default values are for "external callers". Override only when this recurses
 * back to itself. */
void Tensor::print_tensor_initializer(std::ostream &dst, int dim, int offs)
{
	if( data_dim[dim] == 0 )
		return;

	for( int i=0; i<dim; i++)
		dst << "  ";

	dst << "{" ;

	// if this is printing "outer" dimensions, recurse back in till we hit
	// the innermost dimension
	if(   dim < (int)(data_dim.size()-1) ) {
		dst << std::endl;
		for( int i=0; i<data_dim[dim]; i++ )
		{
			print_tensor_initializer(dst, dim+1, offs+i*data_dim[dim+1]);
			if( i <(data_dim[dim]-1) )
				dst << ",";
			dst << std::endl;
		}
	}

	else
		for( int i=0; i<data_dim[dim]; i++)
		{
			int element=offs+i;
			switch(data_type)
			{
				case onnx::TensorProto_DataType_FLOAT:
				{
					float *f = static_cast<float*>(data_buffer);
					dst << std::showpoint << f[element]<< "f";
					break;
				}
				case onnx::TensorProto_DataType_INT32:
				{
					int32_t *f = static_cast<int32_t*>(data_buffer);
					dst << f[element];
					break;
				}
				default:
					ERROR("unimplemented printing of initialized datatype " << data_type_str());

			}
			if( i <(data_dim[dim]-1) )
				dst << ", ";
		}
	dst << "}";
}

void Tensor::print_type_name_dimensions(std::ostream &dst, std::string prefix)
{
	dst << data_type_str() << " ";
	dst << prefix;
	dst << cname();
	for( unsigned i : data_dim )
		dst << "[" << i << "]";
}

