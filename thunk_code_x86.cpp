
struct thunk_code
{
	struct backup_pc
	{
		enum { index = 0 };

		/*
		pop _eip
		*/
		short pop_eip;
		int *_eip;

		backup_pc(int& eip)
		{
			pop_eip = (short)0x058F;
			_eip = &eip;
		}
	};

	struct backup_stack
	{
		enum { index = 1 };

		/*
		mov dword ptr[_esp], esp
		*/
		short save_esp;
		int *_esp;

		backup_stack(int& esp)
		{
			save_esp = (short)0x2589;
			_esp = &esp;
		}
	};

	struct move_stack
	{
		/* forward
		sub esp, direction*4
		push ecx
		push esi
		push edi
		cld
		mov esi, esp+(offset+1)*4
		mov edi, esp+offset*4
		mov ecx, size/4
		rep movsd
		cld
		pop edi
		pop esi
		pop ecx
		*/

		short sub_esp;
		char _inc_size;
		char backup[4];
		char set_esi[3];
		char _src_offset;
		char set_edi[3];
		char _dest_offset;
		char set_count;
		int _count;
		short movs;
		char restore[4];

		move_stack(short offset, int direction, size_t size)
		{
			sub_esp = 0xEC83;
			_inc_size = (char)sizeof(intptr_t);
			_dest_offset = (offset + 3) * sizeof(intptr_t);
			memcpy(backup, "\x51\x57\x56\xFC", 4);
			memcpy(set_esi, "\x8D\x74\x24", sizeof(set_esi));
			memcpy(set_edi, "\x8D\x7C\x24", sizeof(set_edi));
			set_count = 0xB9;
			_count = size / sizeof(intptr_t);
			movs = 0xA5F3;
			memcpy(restore, "\xFC\x5E\x5F\x59", 4);
			if (direction < 0)
			{
				char temp[3];
				backup[3] = '\xFD';
				_dest_offset += size;
				_inc_size = -_inc_size;
				_src_offset = _dest_offset + _inc_size;
				char* code = reinterpret_cast<char*>(this);
				memcpy(temp, code, 3);
				memmove(code, code + 3, sizeof(move_stack) - 3);
				memcpy(code + sizeof(move_stack) - 3, temp, 3);
			}
			else
			{
				_src_offset = _dest_offset + _inc_size;
			}
		}
	};

	struct adjust_params
	{
		char _code[1];

		enum { reg_ecx = '\x4C', reg_edx = '\x54' };

	protected:
		/*
		mov[esp + offset], edx
		*/
		static size_t find_integer_param(size_t argc, size_t& index, const argument_info* arginfos)
		{
			size_t bytes = 0;
			if (arginfos)
			{
				for (; index != argc; index++)
				{
					if (arginfos[index].as_integer())
						break;
					else
						bytes += arginfos[index].stack_size();
				}
			}
			return bytes;
		}

		static char * insert_integer_param(char* code, const argument_info* arginfos,
			char reg, size_t bytes)
		{
			move_stack* _move_stack = reinterpret_cast<move_stack*>(code);
			new(_move_stack) move_stack(0, 1, bytes);
			code = code + sizeof(move_stack);
			*code++ = '\x89';
			*code++ = reg;
			*code++ = '\x24';
			*code++ = (char)bytes;
			return code;
		}

		static char * erase_integer_param(char* code, const argument_info* arginfos,
			char reg, size_t bytes)
		{
			*code++ = '\x8B';
			*code++ = reg;
			*code++ = '\x24';
			*code++ = (char)bytes;
			move_stack* _move_stack = reinterpret_cast<move_stack*>(code);
			new(_move_stack) move_stack(0, -1, bytes);
			code = code + sizeof(move_stack);
			return code;
		}

		static size_t next_integer_params(size_t argc, size_t &index, const argument_info* arginfos)
		{
			if (arginfos)
			{
				for (; index != argc; index++)
				{
					if (arginfos[index].as_integer())
						break;
				}
			}
			return index;
		}
	};

	struct backword_params : public adjust_params
	{
		enum { index = 1 };

		/*
		push edx
		mov edx, ecx
		*/

		explicit backword_params(size_t argc, const argument_info* arginfos)
		{
			assert(argc > 0);
			char * code = _code;
			if (argc == 0) return;

			size_t iargc = 0, index = 0;
			size_t bytes = find_integer_param(argc, index, arginfos);
			if (index != argc)
			{
				++iargc;
				++index;
				bytes += sizeof(intptr_t);
				bytes += find_integer_param(argc, index, arginfos);
			}
			if (index != argc)
			{
				++iargc;
				if (index != 1) code = insert_integer_param(code, arginfos, reg_edx, bytes);
				else *code++ = (char)0x52;
			}
			if (iargc>0)
				*(short*)code = (short)0xD18B;
		}

		static size_t calc_size(size_t argc, const argument_info* arginfos)
		{
			size_t iargc = 0, index = 0;
			size_t bytes = 0;
			index = next_integer_params(argc, index, arginfos);
			if (index != argc)
			{
				++iargc;
				++index;
				index = next_integer_params(argc, index, arginfos);
			}
			if (index != argc)
			{
				++iargc;
				if (index != 1) bytes = sizeof(move_stack) + 4;
			}

			if (bytes>0)
				bytes += 2;
			else
			{
				if (iargc > 1) bytes++;
				if (iargc > 0) bytes += 2;
			}
			return bytes;
		}
	};

	struct forword_params : public adjust_params
	{
		enum { index = 2 };

		/*
		pop edx
		*/

		explicit forword_params(size_t argc, const argument_info* arginfos)
		{
			assert(argc > 0);

			size_t index = 0;
			char* code = _code;
			size_t bytes = find_integer_param(argc, index, arginfos);
			if (index != argc)
			{
				if (index == 0)
				{
					code[0] = (char)0x5A;
				}
				else
				{
					code = erase_integer_param(code, arginfos, reg_edx, bytes);
				}
			}
		}

		static size_t calc_size(size_t argc, const argument_info* arginfos)
		{
			size_t index = 0;
			index = next_integer_params(argc, index, arginfos);
			if (index != argc)
			{
				if (index == 0) return 1;
				else return sizeof(move_stack) + 4;
			}
			return 0;
		}
	};

	struct register_to_stack : public adjust_params
	{
		enum { index = 3 };

		/*
		push edx
		push ecx
		*/

		register_to_stack(size_t argc, const argument_info* arginfos)
		{
			assert(argc > 0);
			char * code = _code;
			if (argc == 0) return;

			size_t iargc = 0, index = 0;
			size_t bytes = find_integer_param(argc, index, arginfos);
			if (index != argc)
			{
				++index;
				if (index != 1) // insert first argument from ecx to stack
					code = insert_integer_param(code, arginfos, reg_ecx, bytes);
				else
				{
					*code++ = (char)0x51;
				}
				bytes += sizeof(intptr_t);
				bytes += find_integer_param(argc, index, arginfos);
				if (index != argc)
				{
					if (index != 1) // insert second arguemnt from edx to stack
						code = insert_integer_param(code, arginfos, reg_edx, bytes);
					else
					{
						code[-1] = (char)0x52;
						*code = (char)0x51;
					}
				}
			}
		}

		static size_t calc_size(size_t argc, const argument_info* arginfos)
		{
			size_t iargc = 0, index = 0;
			size_t bytes = 0;
			index = next_integer_params(argc, index, arginfos);
			if (index != argc)
			{
				++iargc;
				++index;
				if (index>1) bytes += sizeof(move_stack) + 4;
				else bytes++;
				index = next_integer_params(argc, index, arginfos);
				if (index != argc)
				{
					++iargc;
					if (index>1) bytes += sizeof(move_stack) + 4;
					else bytes++;
				}
			}
			return bytes;
		}
	};

	struct pass_this_by_ecx
	{
		enum { index = 4 };

		/*
		mov ecx, _this
		*/
		char mov_this;
		void* _this;

		pass_this_by_ecx()
		{
			mov_this = (char)0xB9;
		}
	};

	struct pass_this_by_stack
	{
		enum { index = 5 };

		/*
		push _this
		*/
		char push_this;
		void* _this;

		pass_this_by_stack()
		{
			push_this = (char)0x68;
		}
	};

	struct restore_stack
	{
		enum { index = 9 };

		/*
		sub esp, n
		*/
		short restore_esp;
		size_t n;

		restore_stack(size_t bytes)
		{
			restore_esp = (short)0xEC81;
			n = bytes;
		}
	};

	struct call_function
	{
		enum { index = 7 };

		/*
		call _proc
		*/
		char call;
		uintptr_t _proc;

		call_function()
		{
			call = (char)0xE8;
		}
	};

	struct jump_function
	{
		enum { index = 6 };

		/*
		jump _proc
		*/
		char jump;
		uintptr_t _proc;

		jump_function()
		{
			jump = (char)0xE9;
		}
	};

	struct restore_pc
	{
		enum { index = 8 };

		/*
		push _eip
		*/
		short push_eip;
		int *_eip;

		restore_pc(int& eip)
		{
			push_eip = (short)0x35FF;
			_eip = &eip;
		}
	};

	struct return_caller
	{
		enum { index = 10 };

		/*
		ret (n)
		*/
		char ret;
		short _n;

		return_caller()
		{
			ret = (char)0xC3;
		}
		explicit return_caller(short bytes)
		{
			ret = (char)0xC2;
			_n = bytes;
		}

		static size_t calc_size(size_t n)
		{
			if (n == 0) return 1;
			else return sizeof(return_caller);
		}
	};

	pass_this_by_ecx* _pass_this_by_ecx;
	pass_this_by_stack* _pass_this_by_stack;
	jump_function* _jump_function;
	call_function* _call_function;

	int _eip;

	enum { code_count = 11 };
	static const bool enabled_codes[3][4][code_count];

	static size_t calc_size(call_declare caller, call_declare callee, size_t argc, const argument_info* arginfos)
	{
		static const size_t code_size[code_count] = {
			sizeof(backup_pc),
			sizeof(backword_params),
			sizeof(forword_params),
			sizeof(register_to_stack),
			sizeof(pass_this_by_ecx),
			sizeof(pass_this_by_stack),
			sizeof(jump_function),
			sizeof(call_function),
			sizeof(restore_pc),
			sizeof(restore_stack),
			sizeof(return_caller)
		};
		if (caller == cc_thiscall) throw bad_call();

		size_t size = 0;
		for (size_t i = 0; i != code_count; i++)
		{
			if (enabled_codes[caller][callee][i])
			{
				if (i == backword_params::index)
					size += backword_params::calc_size(argc, arginfos);
				else if (i == forword_params::index)
					size += forword_params::calc_size(argc, arginfos);
				else if (i == register_to_stack::index)
					size += register_to_stack::calc_size(argc, arginfos);
				else
					size += code_size[i];

			}
		}
		if (size == 0) throw bad_call();

		return sizeof(thunk_code) + size;
	}

	thunk_code(call_declare caller, call_declare callee, size_t argc, const argument_info* arginfos)
	{
		memset(this, 0, sizeof(thunk_code));

		char* code = reinterpret_cast<char*>(this + 1);
		size_t param_bytes = 0;
		if (arginfos)
		{
			for (size_t i = 0; i != argc; i++)
				param_bytes += arginfos[i].stack_size();
		}
		else
		{
			param_bytes = argc * sizeof(intptr_t);
		}
		if (enabled_codes[caller][callee][backup_pc::index])
		{
			backup_pc* _backup_pc = reinterpret_cast<backup_pc*>(code);
			new(_backup_pc) backup_pc(_eip);
			code += sizeof(backup_pc);
		}
		if (enabled_codes[caller][callee][backword_params::index])
		{
			size_t c = backword_params::calc_size(argc, arginfos);
			if (c > 0)
			{
				backword_params* _backword_params = reinterpret_cast<backword_params*>(code);
				new(_backword_params) backword_params(argc, arginfos);
				code += c;
			}
		}
		if (enabled_codes[caller][callee][forword_params::index])
		{
			size_t c = forword_params::calc_size(argc, arginfos);
			if (c > 0)
			{
				forword_params* _forword_params = reinterpret_cast<forword_params*>(code);
				new(_forword_params) forword_params(argc, arginfos);
				code += c;
			}
		}
		if (enabled_codes[caller][callee][register_to_stack::index])
		{
			size_t c = register_to_stack::calc_size(argc, arginfos);
			if (c > 0)
			{
				register_to_stack* _register_to_stack = reinterpret_cast<register_to_stack*>(code);
				new(_register_to_stack) register_to_stack(argc, arginfos);
				code += c;
			}
		}
		if (enabled_codes[caller][callee][pass_this_by_ecx::index])
		{
			_pass_this_by_ecx = reinterpret_cast<pass_this_by_ecx*>(code);
			new(_pass_this_by_ecx) pass_this_by_ecx();
			code += sizeof(pass_this_by_ecx);
		}
		if (enabled_codes[caller][callee][pass_this_by_stack::index])
		{
			_pass_this_by_stack = reinterpret_cast<pass_this_by_stack*>(code);
			new(_pass_this_by_stack) pass_this_by_stack();
			code += sizeof(pass_this_by_stack);
		}
		if (enabled_codes[caller][callee][jump_function::index])
		{
			if (enabled_codes[caller][callee][restore_pc::index])
			{
				restore_pc* _restore_pc = reinterpret_cast<restore_pc*>(code);
				new(_restore_pc) restore_pc(_eip);
				code += sizeof(restore_pc);
			}

			_jump_function = reinterpret_cast<jump_function*>(code);
			new(_jump_function) jump_function();
			code += sizeof(jump_function);
		}
		if (enabled_codes[caller][callee][call_function::index])
		{
			_call_function = reinterpret_cast<call_function*>(code);
			new(_call_function) call_function();
			code += sizeof(call_function);

			if (enabled_codes[caller][callee][restore_stack::index])
			{
				restore_stack* _restore_stack = reinterpret_cast<restore_stack*>(code);
				new(_restore_stack) restore_stack(param_bytes);
				code += sizeof(restore_stack);
			}
			if (enabled_codes[caller][callee][restore_pc::index])
			{
				restore_pc* _restore_pc = reinterpret_cast<restore_pc*>(code);
				new(_restore_pc) restore_pc(_eip);
				code += sizeof(restore_pc);
			}
			if (enabled_codes[caller][callee][return_caller::index])
			{
				return_caller* _return_caller = reinterpret_cast<return_caller*>(code);
				if (caller != cc_cdecl && callee == cc_cdecl)
					new(_return_caller) return_caller(param_bytes + sizeof(intptr_t));
				else if (caller == cc_cdecl && callee == cc_cdecl)
					new(_return_caller) return_caller(sizeof(intptr_t));
				else
					new(_return_caller) return_caller();
				code += sizeof(return_caller);
			}
		}
	}

	void bind(void* object, void* proc)
	{
		if (_pass_this_by_ecx)
			_pass_this_by_ecx->_this = object;
		else if (_pass_this_by_stack)
			_pass_this_by_stack->_this = object;

		if (_call_function)
			_call_function->_proc = (intptr_t)proc - (intptr_t)(_call_function + 1);
		if (_jump_function)
			_jump_function->_proc = (intptr_t)proc - (intptr_t)(_jump_function + 1);
	}

};

const bool thunk_code::enabled_codes[3][4][code_count] = {
	// backup_pc	backword_params	forword_params	register_to_stack	pass_this_by_ecx	pass_this_by_stack	jump_function	call_function	restore_pc	restore_stack	return_caller
	{ //fastcall -> fastcall
		{ true,		true,			false,			false,				true,				false,				true,			false,			true,		false,			false },
		//fastcall -> _cdecl
		{ true,		false,			false,			true,				false,				true,				false,			true,			true,		false,			true },
		//fastcall -> stdcall
		{ true,		false,			false,			true,				false,				true,				true,			false,			true,		false,			false },
		//fastcall -> thiscall
		{ true,		false,			false,			true,				true,				false,				true,			false,			true,		false,			false },
	},
	{	//_cdecl -> fastcall
		{ true,		false,			true,			false,				true,				false,				false,			true,			true,		true,			true },
		//_cdecl -> _cdecl
		{ true,		false,			false,			false,				false,				true,				false,			true,			true,		false,			true },
		//_cdecl -> stdcall
		{ true,		false,			false,			false,				false,				true,				false,			true,			true,		true,			true },
		//_cdecl -> thiscall
		{ true,		false,			false,			false,				true,				false,				false,			true,			true,		true,			true }
	},
	{	//stdcall -> fastcall
		{ true,		false,			true,			false,				true,				false,				true,			false,			true,		false,			false },
		//stdcall -> _cdecl
		{ true,		false,			false,			false,				false,				true,				false,			true,			true,		false,			true },
		//stdcall -> stdcall
		{ true,		false,			false,			false,				false,				true,				true,			false,			true,		false,			false },
		//stdcall -> thiscall
		{ false,	false,			false,			false,				true,				false,				true,			false,			false,		false,			false }
	}

};
