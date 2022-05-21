/* This file is part of onnx2c.
 *
 * GlobalAveragePool node.
 */ 
namespace toC {

class GlobalAveragePool : public Node {
	public:
	GlobalAveragePool() {
		op_name = "GlobalAveragePool";
		X=Y=NULL;
	}

	std::vector<float> an_attribute;

	const Tensor *X;
	const Tensor *Y;


	virtual void print_parameters(std::ostream &dst, bool decorate ) const override
	{
		X->print_tensor_as_const(dst, !decorate);
		dst << ", ";

		Y->print_tensor(dst, !decorate);
	}

	/* Body of the node implementing function */
	virtual void print(std::ostream &dst) const override
	{
		int batch_size = X->data_dim[0];
		int num_channels = X->data_dim[1];

		dst << "\t/* GlobalAveragePool */" << std::endl;
		dst<<"\t"  << "for( int32_t b=0; b<" << batch_size << "; b++ ) {" << std::endl;
		dst<<"\t"  << "for( int32_t c=0; c<" << num_channels << "; c++ ) {" << std::endl;

		// TODO: float16, double? accuracy vs speed...
		dst << "\t\tfloat dimsum=0.0f;" << std::endl;

		int dim_num_elem=1; // number of elements averaged over
		std::string in_idx_string = X->cname() + "[b][c]"; // start of the input element access string 
		std::string out_idx_string = Y->cname() + "[b][c]"; // start of the input element access string 

		for( unsigned dim = 2; dim < X->data_dim.size(); dim ++ ) {
			int dim_size = X->data_dim[dim];
			dim_num_elem *= dim_size;

			std::string dim_var = "d" + std::to_string(dim-2);
			in_idx_string += "[" + dim_var + "]";
			out_idx_string += "[0]";
			dst << "\t\t" << "for( int32_t " << dim_var <<" = 0; ";
			dst <<            dim_var << "<" << dim_size << "; ";
			dst <<            dim_var<<"++ ) {" << std::endl;
		}

			dst << "\t\t\tdimsum +=  " << in_idx_string << ";" << std::endl;

		for( unsigned dim = 2; dim < X->data_dim.size(); dim ++ ) {
			dst << "\t\t}" << std::endl;
		}

		dst << "\t\t" << out_idx_string << " = dimsum / " << dim_num_elem << ";" << std::endl; 

		// close loop over b and c
		dst<<"\t}" << std::endl;
		dst<<"\t}" << std::endl;
	}


	virtual void resolve(void) override
	{
		X = inputs[0];
		if(  typeConstraint_plainFloatingPoints(X) == false )
			ERROR("Incorrect input for node"); 


		/* Create output tensors */
		Tensor *rv = new Tensor;
		rv->data_dim.push_back(X->data_dim[0]);
		rv->data_dim.push_back(X->data_dim[1]);
		for( unsigned i=2; i<X->data_dim.size(); i++)
			rv->data_dim.push_back(1);
		rv->data_type = X->data_type;
		Y=rv;
		outputs.push_back(rv);

	}
};
}

