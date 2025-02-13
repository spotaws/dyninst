/*
 * See the dyninst/COPYRIGHT file for copyright information.
 *
 * We provide the Paradyn Tools (below described as "Paradyn")
 * on an AS IS basis, and do not warrant its validity or performance.
 * We reserve the right to update, modify, or discontinue this
 * software at any time.  We shall have no obligation to supply such
 * updates or modifications or any other form of support to you.
 *
 * By your use of Paradyn, you understand and agree that we (or any
 * other person or entity with proprietary rights in Paradyn) are
 * under no obligation to provide either maintenance services,
 * update services, notices of latent defects, or correction of
 * defects for Paradyn.
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 */

#include "Register.h"
#include "../../common/src/Singleton.h"
#include <vector>
#include <set>
#include <sstream>
#include "Visitor.h"
#include "../../common/src/singleton_object_pool.h"
#include "InstructionDecoder-power.h"
#include "dyn_regs.h"
#include "ArchSpecificFormatters.h"
#include "../../common/h/compiler_diagnostics.h"

using namespace std;

extern bool ia32_is_mode_64();


namespace Dyninst
{
  namespace InstructionAPI
  {
    RegisterAST::RegisterAST(MachRegister r, unsigned int lowbit, unsigned int highbit ,uint32_t num_elements) :
            Expression(r), m_Reg(r), m_Low(lowbit), m_High(highbit) , m_num_elements(num_elements)
    {
    }
    RegisterAST::RegisterAST(MachRegister r, uint32_t num_elements) :
            Expression(r), m_Reg(r), m_Low(0), m_num_elements(num_elements)
    {

        m_High = r.size() * 8;
    }

    RegisterAST::RegisterAST(MachRegister r, unsigned int lowbit, unsigned int highbit, Result_Type regType, uint32_t num_elements):
			Expression(regType), m_Reg(r), m_Low(lowbit), m_High(highbit), m_num_elements(num_elements)
    {
	}

    RegisterAST::~RegisterAST()
    {
    }
    void RegisterAST::getChildren(vector<InstructionAST::Ptr>& /*children*/) const
    {
      return;
    }
    void RegisterAST::getChildren(vector<Expression::Ptr>& /*children*/) const
    {
        return;
    }
    void RegisterAST::getUses(set<InstructionAST::Ptr>& uses)
    {
        uses.insert(shared_from_this());
        return;
    }
    bool RegisterAST::isUsed(InstructionAST::Ptr findMe) const
    {
        return findMe->checkRegID(m_Reg, m_Low, m_High);
    }
    MachRegister RegisterAST::getID() const
    {
      return m_Reg;
    }

    std::string RegisterAST::format(Architecture arch, formatStyle f) const
    {
        if(arch == Arch_amdgpu_vega || arch == Arch_amdgpu_gfx908 || arch == Arch_amdgpu_gfx90a){
            return RegisterAST::format(f);
        }
        return ArchSpecificFormatter::getFormatter(arch).formatRegister(m_Reg.name());
    }

    std::string RegisterAST::format(formatStyle) const
    {
        std::string name = m_Reg.name();
        std::string::size_type substr = name.rfind("::");
        if(substr != std::string::npos)
        {
            name = name.substr(substr + 2, name.length());
        }
        if( m_num_elements ==0 ){
            return "";
        }else if ( m_num_elements > 1){
            uint32_t id = m_Reg & 0xff ;
            uint32_t regClass = m_Reg.regClass();
 
            uint32_t  size = m_num_elements;

DYNINST_DIAGNOSTIC_BEGIN_SUPPRESS_LOGICAL_OP

            if(regClass == amdgpu_gfx908::SGPR || regClass == amdgpu_gfx90a::SGPR || regClass == amdgpu_vega::SGPR){
                return "S["+to_string(id) + ":" + to_string(id+size-1)+"]";
            }

            if(regClass == amdgpu_gfx908::VGPR || regClass == amdgpu_gfx90a::VGPR || regClass == amdgpu_vega::VGPR){
                return "V["+to_string(id) + ":" + to_string(id+size-1)+"]";
            }

            if(regClass == amdgpu_gfx908::ACC_VGPR || regClass == amdgpu_gfx90a::ACC_VGPR){
                return "ACC["+to_string(id) + ":" + to_string(id+size-1)+"]";
            }

DYNINST_DIAGNOSTIC_END_SUPPRESS_LOGICAL_OP

            if(m_Reg == amdgpu_gfx908::vcc_lo || m_Reg == amdgpu_gfx90a::vcc_lo || m_Reg == amdgpu_vega::vcc_lo)
                return "VCC";
            if(m_Reg == amdgpu_gfx908::exec_lo || m_Reg == amdgpu_gfx90a::exec_lo || m_Reg == amdgpu_vega::exec_lo)
                return "EXEC";

       
        }else if ( m_High -m_Low > 32 && m_Reg.size()*8 != m_High - m_Low){

        // Size of base register * 8 != m_High - mLow ( in bits) when we it is a register vector
            uint32_t id = m_Reg & 0xff ;
            uint32_t regClass = m_Reg.regClass();
            uint32_t size = (m_High - m_Low ) / 32;

            // Suppress warning (for compilers where it is a false positive)
            // The values of the two *::SGPR constants are identical, as
            // are the two *::VGPR constants
DYNINST_DIAGNOSTIC_BEGIN_SUPPRESS_LOGICAL_OP

            if(regClass == amdgpu_gfx908::SGPR || regClass == amdgpu_gfx90a::SGPR || regClass == amdgpu_vega::SGPR){
                return "S["+to_string(id) + ":" + to_string(id+size-1)+"]";
            }

            if(regClass == amdgpu_gfx908::VGPR || regClass == amdgpu_gfx90a::VGPR || regClass == amdgpu_vega::VGPR){
                return "V["+to_string(id) + ":" + to_string(id+size-1)+"]";
            }

            if(regClass == amdgpu_gfx908::ACC_VGPR || regClass == amdgpu_gfx90a::ACC_VGPR){
                return "ACC["+to_string(id) + ":" + to_string(id+size-1)+"]";
            }

DYNINST_DIAGNOSTIC_END_SUPPRESS_LOGICAL_OP

            if(m_Reg == amdgpu_gfx908::vcc_lo || m_Reg == amdgpu_gfx90a::vcc_lo || m_Reg == amdgpu_vega::vcc_lo)
                return "VCC";
            if(m_Reg == amdgpu_gfx908::exec_lo || m_Reg == amdgpu_gfx90a::exec_lo || m_Reg == amdgpu_vega::exec_lo)
                return "EXEC";

            name +=  "["+to_string(m_Low)+":"+to_string(m_High)+"]";
        }
        /* we have moved to AT&T syntax (lowercase registers) */
        for(char &c : name) c = std::toupper(c);

        return name;
    }
      std::string MaskRegisterAST::format(Architecture, formatStyle f) const
      {
          return format(f);
      }

    std::string MaskRegisterAST::format(formatStyle) const
    {
        std::string name = m_Reg.name();
        std::string::size_type substr = name.rfind(':');
        if(substr != std::string::npos)
        {
            name = name.substr(substr + 1, name.length());
        }

        /* The syntax for a masking register is {kX} in AT&T syntax. */
        return  "{" + name + "}";
    }

     

    RegisterAST RegisterAST::makePC(Dyninst::Architecture arch)
    {
        return RegisterAST(MachRegister::getPC(arch), 0, MachRegister::getPC(arch).size());
    }

    bool RegisterAST::operator<(const RegisterAST& rhs) const
    {
        if(m_Reg < rhs.m_Reg) return true;
        if(rhs.m_Reg < m_Reg) return false;
        if(m_Low < rhs.m_Low) return true;
        if(m_Low > rhs.m_Low) return false;
        return m_High < rhs.m_High;
    }
    bool RegisterAST::isStrictEqual(const InstructionAST& rhs) const
    {
      if(rhs.checkRegID(m_Reg, m_Low, m_High))
        {
	  const RegisterAST& rhs_reg = dynamic_cast<const RegisterAST&>(rhs);
	  if ((m_Low == rhs_reg.m_Low) && (m_High == rhs_reg.m_High)) {
	    return true;
	  }
	  else {
	    return false;
	  }
        }
      return false;
    }
    RegisterAST::Ptr RegisterAST::promote(const InstructionAST::Ptr regPtr) {
		const RegisterAST::Ptr r = boost::dynamic_pointer_cast<RegisterAST>(regPtr);
        return RegisterAST::promote(r.get());
    }
    MachRegister RegisterAST::getPromotedReg() const
    {
        return m_Reg.getBaseRegister();
    }

    RegisterAST::Ptr RegisterAST::promote(const RegisterAST* regPtr) {
        if (!regPtr) return RegisterAST::Ptr();

        // We want to upconvert the register ID to the maximal containing
        // register for the platform - either EAX or RAX as appropriate.

        return make_shared(singleton_object_pool<RegisterAST>::construct(regPtr->getPromotedReg(), 0,
                           regPtr->getPromotedReg().size()));
    }
    bool RegisterAST::isFlag() const
    {
        return m_Reg.getBaseRegister() == x86::flags;
    }
    bool RegisterAST::checkRegID(MachRegister r, unsigned int low, unsigned int high) const
    {
#if defined(DEBUG)
      fprintf(stderr, "%s (%d-%d/%x) compared with %s (%d-%d/%x)",
                    format().c_str(), m_Low, m_High, m_Reg.getBaseRegister().val(),
                    r.name().c_str(), low, high, r.getBaseRegister().val());
#endif
        return (r.getBaseRegister() == m_Reg.getBaseRegister()) && (low <= m_High) &&
                (high >= m_Low);
    }
    void RegisterAST::apply(Visitor* v)
    {
        v->visit(this);
    }
    bool RegisterAST::bind(Expression* e, const Result& val)
    {
        if(Expression::bind(e, val)) {
            return true;
        }
	//        fprintf(stderr, "checking %s against %s with checkRegID in RegisterAST::bind... %p", e->format().c_str(),
	//format().c_str(), this);
        if(e->checkRegID(m_Reg, m_Low, m_High))
        {
	  //fprintf(stderr, "yes\n");
            setValue(val);
            return true;
        }
        //fprintf(stderr, "no\n");
        return false;
    }
  }
}
