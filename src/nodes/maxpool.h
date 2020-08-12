#include <cmath>

namespace toC {

class MaxPool : public Node {
	public:
	MaxPool() {
		op_name = "MaxPool";
		ceil_mode = 0;
		storage_order=0;
	}
	/* MaxPool node specific attributes */
	int ceil_mode;
	std::vector<int> dilations;
	std::vector<int> kernel_shape;
	std::vector<int> pads;
	int storage_order;
	std::vector<int> strides;

	std::vector<int> pad_shapes; // pad_shapes[i] = "sum of pads along axis i"

	void parseAttributes_auto_pad( const onnx::AttributeProto &a ) {
		if (a.s() != "NOTSET" )
			ERROR("Unimplemented: MaxPool auto_pad is deprecated, and not set to NOTSET");
	}

	void parseAttributes_ceil_mode( const onnx::AttributeProto &a ) {
		if( a.type() != onnx::AttributeProto_AttributeType_INT )
			ERROR("Wrong attribute type for MaxPool attribute 'ceil_mode'");
		ceil_mode = a.i();
	}

	void parseAttributes_dilations( const onnx::AttributeProto &a ) {
		if( a.type() != onnx::AttributeProto_AttributeType_INTS )
			ERROR("Wrong attribute type for MaxPool attribute 'dilations'");

		for( auto i : a.ints() ) {
			dilations.push_back(i);
			if( i != 1 )
				ERROR("Unimplemented: dilations other than 1");
		}
	}


	void parseAttributes_kernel_shape( const onnx::AttributeProto &a ) {
		if( a.type() != onnx::AttributeProto_AttributeType_INTS )
			ERROR("Wrong attribute type for MaxPool attribute 'kernel_shape'");

		for( auto i : a.ints() ) {
			kernel_shape.push_back(i);
		}
	}

	void parseAttributes_pads( const onnx::AttributeProto &a ) {
		if( a.type() != onnx::AttributeProto_AttributeType_INTS )
			ERROR("Wrong attribute type for MaxPool attribute 'pads'");

		for( auto i : a.ints() ) {
			pads.push_back(i);
		}
	}

	void parseAttributes_storage_order( const onnx::AttributeProto &a ) {
		if( a.type() != onnx::AttributeProto_AttributeType_INT )
			ERROR("Wrong attribute type for MaxPool attribute 'storage_order'");
		storage_order = a.i();
	}

	void parseAttributes_strides( const onnx::AttributeProto &a ) {
		if( a.type() != onnx::AttributeProto_AttributeType_INTS )
			ERROR("Wrong attribute type for MaxPool attribute 'kernel_strides'");

		for( auto i : a.ints() ) {
			strides.push_back(i);
		}
	}

	virtual void parseAttributes( onnx::NodeProto &node ) override {

		for( const auto a : node.attribute() ) {
			if( a.name() == "auto_pad" )
				parseAttributes_auto_pad(a);
			else if( a.name() == "ceil_mode" )
				parseAttributes_ceil_mode(a);
			else if( a.name() == "dilations" )
				parseAttributes_dilations(a);
			else if( a.name() == "kernel_shape" )
				parseAttributes_kernel_shape(a);
			else if( a.name() == "pads" )
				parseAttributes_pads(a);
			else if( a.name() == "storage_order" )
				parseAttributes_pads(a);
			else if( a.name() == "strides" )
				parseAttributes_strides(a);
		}
	}

	virtual void print(std::ostream &dst) const
	{
		if( inputs.size() != 1 )
			ERROR("wrong number of inputs to MaxPool");
		if( outputs.size() != 1 )
			ERROR("wrong number of outputs from MaxPool");

		dst << "\t/* MaxPool" << std::endl;
		dst << "\t *" << std::endl;
		dst << "\t * ceil_mode: " << ceil_mode <<std::endl;
		dst << "\t * dilations: ";
			for( int d: dilations )
				dst << d << " ";
		dst << std::endl;
		dst << "\t * kernel_shape: ";
			for( int k: kernel_shape )
				dst << k << " ";
		dst << std::endl;
		dst << "\t * pads: ";
			for( int p: pads)
				dst << p << " ";
		dst << std::endl;
		dst << "\t * storage_order: " << storage_order <<std::endl;
		dst << "\t * strides: ";
			for( int s: strides)
				dst << s << " ";
		dst << std::endl <<  "\t */" << std::endl;

		/* TODO: move to resolve? */
		for(auto p: pads)
			if(p!=0)
				ERROR("Unimplemented: pads != 0");
		int data_dim = inputs[0]->data_dim.size()-2;
		for(int i=0; i<data_dim; i++) {
			int out_size = outputs[0]->data_dim[2+i];
			int in_size = inputs[0]->data_dim[2+i];
			if( (((out_size-1)*strides[i]) + kernel_shape[i]) > in_size )
				ERROR("Sizes mismatch. Padding would have been necessary? Node" << onnx_name);
		}
		// at this point we know:
		// no padding is needed
		// just looping through input with given kerne size+stride size does not overflow reads
		// only 2D data supported for now

		int batch_size = inputs[0]->data_dim[0];
		int channels = inputs[0]->data_dim[1];
		int d1_out = outputs[0]->data_dim[2];
		int d2_out = outputs[0]->data_dim[3];
		std::string type = inputs[0]->data_type_str();
		std::string in = inputs[0]->cname();
		std::string out = outputs[0]->cname();

		dst<<"\t"        << "for( int32_t b=0; b<" << batch_size << "; b++ ) {" << std::endl;
		dst<<"\t\t"      <<   "for( int32_t c=0; c<" << channels << "; c++ ) {" << std::endl;

		dst<<"\t\t\t"    <<     "for( int32_t o1=0; o1 < " << d1_out << "; o1++) {" << std::endl;
		dst<<"\t\t\t"    <<     "for( int32_t o2=0; o2 < " << d2_out << "; o2++) {" << std::endl;

		dst<<"\t\t\t\t"  <<       "int in1 = o1*"<<strides[0]<<";"<<std::endl;
		dst<<"\t\t\t\t"  <<       "int in2 = o2*"<<strides[1]<<";"<<std::endl;

		// TODO: the trailing "][0][0]" assumes 2D input
		dst<<"\t\t\t\t"  <<       type << " curmax = " << in<< "[b][c][in1][in2];" << std::endl;
		dst<<"\t\t\t\t"  <<       "for( int32_t k1=0; k1 < " << kernel_shape[0] << "; k1++) {" << std::endl;
		dst<<"\t\t\t\t"  <<       "for( int32_t k2=0; k2 < " << kernel_shape[1] << "; k2++) {" << std::endl;

		dst<<"\t\t\t\t\t"<<         "curmax = MAX( curmax, " << in << "[b][c][in1+k1][in2+k2]);" <<std::endl;

		dst<<"\t\t\t\t}" << std::endl;
		dst<<"\t\t\t\t}" << std::endl;

		dst<<"\t\t\t\t"  <<       out << "[b][c][o1][o2] = curmax;" << std::endl;

		dst<<"\t\t\t}" << std::endl;
		dst<<"\t\t\t}" << std::endl;
		dst<<"\t\t}" << std::endl;
		dst<<"\t}" << std::endl;
		

		dst << std::endl;
	}
 
	virtual void resolveOutput(const std::vector< const Tensor*> &inputs, std::vector<Tensor *> &outputs)
	{
		const Tensor *x = inputs[0];

		if( !(  typeConstraint_plainFloatingPoints(x)
		      ||typeConstraint_8bit(x)) )
			ERROR("Incorrect input for node"); 

		if( x->data_dim.size() != 4 )
			ERROR("Unimplemented: MaxPool for non 2D images");
		if( x->data_dim[0] != 1 )
			ERROR("Unimplemented: MaxPool batches bigger than 1");


		if( kernel_shape.size() == 0 )
			ERROR("MaxPool: kernel_shape not provided");

		if( storage_order != 0 )
			ERROR("Unimplemented: column-major storage_order");

		if( strides.size() == 0 )
			for( unsigned i=0; i< x->data_dim.size(); i++ )
				strides.push_back(1);


		unsigned data_dims = x->data_dim.size()-2;
		// if 'pads' attribute not given, fill with defaults
		if( pads.size() == 0 ) {
			for(unsigned i=0; i<data_dims; i++) {
				pads.push_back(0);
				pads.push_back(0);
			}
		}
		if( pads.size() != 2*data_dims ) {
			ERROR("Pads size mismatch!");
		}
		for( unsigned i=0; i<data_dims; i++ ) {
			pad_shapes.push_back(pads[i]+pads[data_dims+i]);
		}

		Tensor *rv = new Tensor;

		rv->data_dim.push_back(x->data_dim[0]); //batch size
		rv->data_dim.push_back(x->data_dim[1]); //num channels

		uint64_t rv_num_elem = x->data_dim[0] * x->data_dim[1];
		for( unsigned i=2; i<x->data_dim.size(); i++ ) {
			int d;
			int in_dim = x->data_dim[i];
			int pad = pad_shapes[i-2];
			int kernel = kernel_shape[i-2];
			int dilation = dilations.size()==0 ? 1 : dilations[i-2];
			int stride = strides[i-2];
			// these formulas are straight from the spec!
			if (ceil_mode)
				d = ceil((float)(in_dim + pad - ((kernel - 1) * dilation + 1)) / stride + 1);
			else
				d = floor((float)(in_dim + pad - ((kernel  - 1) * dilation + 1)) / stride + 1);

			rv->data_dim.push_back(d);
			rv_num_elem *= d;
		};

		rv->data_type = x->data_type;
		rv->data_num_elem = rv_num_elem;
		outputs.push_back(rv);
		//TODO: also push out the optional Indices tensor. But this needs some fixes in Graph. And a test case
		//      that uses multiple outputs.
	}
};
}
