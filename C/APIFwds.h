// Copyright (c) 2016/17/18/19/20/21 Leandro T. C. Melo <ltcmelo@gmail.com>
// Copyright (c) 2008 Roberto Raggi <roberto.raggi@gmail.com>
//
// Permission is hereby granted, free of charge, to any person obtaining a copy
// of this software and associated documentation files (the "Software"), to deal
// in the Software without restriction, including without limitation the rights
// to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
// copies of the Software, and to permit persons to whom the Software is
// furnished to do so, subject to the following conditions:
//
// The above copyright notice and this permission notice shall be included in
// all copies or substantial portions of the Software.
//
// THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
// IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
// FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
// AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
// LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
// OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
// THE SOFTWARE.

#ifndef PSYCHE_C_FWDS_H__
#define PSYCHE_C_FWDS_H__

namespace psy {
namespace C {

class MemoryPool;
class SyntaxTree;
class Compilation;

//=================================================================== Tokens

class SyntaxToken;

class SyntaxLexeme;
class Identifier;
class IntegerConstant;
class FloatingConstant;
class CharacterConstant;
class StringLiteral;

//=================================================================== Nodes

class SyntaxNode;
class SyntaxNodeList;
class SyntaxVisitor;

template <class SyntaxNodeT, class DerivedListT> class CoreSyntaxNodeList;
template <class SyntaxNodeT> class SyntaxNodePlainList;
template <class SyntaxNodeT> class SyntaxNodeSeparatedList;

//--------------//
// Declarations //
//--------------//
class TranslationUnitSyntax;
class DeclarationSyntax;
class IncompleteDeclarationSyntax;
class NamedDeclarationSyntax;
class TypeDeclarationSyntax;
class TagDeclarationSyntax;
class StructOrUnionDeclarationSyntax;
class EnumDeclarationSyntax;
class EnumMemberDeclarationSyntax;
class DeclaratorDeclarationSyntax;
class ValueDeclarationSyntax;
class DeclaratorDeclarationSyntax;
class VariableAndOrFunctionDeclarationSyntax;
class ParameterDeclarationSyntax;
class FieldDeclarationSyntax;
class StaticAssertDeclarationSyntax;
class FunctionDefinitionSyntax;
class ExtGNU_AsmStatementDeclarationSyntax;
class ExtPSY_TemplateDeclarationSyntax;

/* Specifiers */
class SpecifierSyntax;
class TrivialSpecifierSyntax;
class StorageClassSyntax;
class BuiltinTypeSpecifierSyntax;
class TaggedTypeSpecifierSyntax;
class TypeDeclarationAsSpecifierSyntax;
class AtomicTypeSpecifierSyntax;
class TypeQualifierSyntax;
class FunctionSpecifierSyntax;
class AlignmentSpecifierSyntax;
class TypedefNameSyntax;
class ExtGNU_TypeofSyntax;
class ExtGNU_AttributeSpecifierSyntax;
class ExtGNU_AttributeSyntax;
class ExtGNU_AsmLabelSyntax;
class ExtPSY_QuantifiedTypeSpecifierSyntax;

/* Declarators */
class DeclaratorSyntax;
class IdentifierDeclaratorSyntax;
class ParenthesizedDeclaratorSyntax;
class PointerDeclaratorSyntax;
class AbstractDeclaratorSyntax;
class ArrayOrFunctionDeclaratorSyntax;
class BitfieldDeclaratorSyntax;
class DeclaratorSuffixSyntax;
class SubscriptSuffixSyntax;
class ParameterSuffixSyntax;

/* Initializers */
class InitializerSyntax;
class ExpressionInitializerSyntax;
class BraceEnclosedInitializerSyntax;
class DesignatedInitializerSyntax;
class DesignatorSyntax;
class ArrayDesignatorSyntax;
class FieldDesignatorSyntax;

//-------------//
// Expressions //
//-------------//
class ExpressionSyntax;
class IdentifierExpressionSyntax;
class ConstantExpressionSyntax;
class StringLiteralExpressionSyntax;
class ParenthesizedExpressionSyntax;
class GenericSelectionExpressionSyntax;
class GenericAssociationSyntax;
class ExtGNU_EnclosedCompoundStatementExpressionSyntax;

/* Operations */
class UnaryExpressionSyntax;
class PostfixUnaryExpressionSyntax;
class PrefixUnaryExpressionSyntax;
class MemberAccessExpressionSyntax;
class ArraySubscriptExpressionSyntax;
class TypeTraitExpressionSyntax;
class CallExpressionSyntax;
class CompoundLiteralExpressionSyntax;
class CastExpressionSyntax;
class BinaryExpressionSyntax;
class ConditionalExpressionSyntax;
class AssignmentExpressionSyntax;
class SequencingExpressionSyntax;

//------------//
// Statements //
//------------//
class StatementSyntax;
class CompoundStatementSyntax;
class DeclarationStatementSyntax;
class ExpressionStatementSyntax;
class LabeledStatementSyntax;
class IfStatementSyntax;
class SwitchStatementSyntax;
class WhileStatementSyntax;
class DoStatementSyntax;
class ForStatementSyntax;
class GotoStatementSyntax;
class ContinueStatementSyntax;
class BreakStatementSyntax;
class ReturnStatementSyntax;
class ExtGNU_AsmStatementSyntax;
class ExtGNU_AsmQualifierSyntax;
class ExtGNU_AsmOperandSyntax;

//--------//
// Common //
//--------//
class TypeNameSyntax;
class TypeReferenceSyntax;
class ExpressionAsTypeReferenceSyntax;
class TypeNameAsTypeReferenceSyntax;

//-------------//
// Ambiguities //
//-------------//
class AmbiguousTypeNameOrExpressionAsTypeReferenceSyntax;
class AmbiguousCastOrBinaryExpressionSyntax;
class AmbiguousExpressionOrDeclarationStatementSyntax;

/* Lists */
typedef SyntaxNodePlainList<DeclarationSyntax*> DeclarationListSyntax;
typedef SyntaxNodeSeparatedList<EnumMemberDeclarationSyntax*> EnumMemberListSyntax;
typedef SyntaxNodeSeparatedList<ParameterDeclarationSyntax*> ParameterDeclarationListSyntax;
typedef SyntaxNodePlainList<SpecifierSyntax*> SpecifierListSyntax;
typedef SyntaxNodeSeparatedList<ExtGNU_AttributeSyntax*> ExtGNU_AttributeListSyntax;
typedef SyntaxNodeSeparatedList<DeclaratorSyntax*> DeclaratorListSyntax;
typedef SyntaxNodePlainList<DeclaratorSuffixSyntax*> DeclaratorSuffixListSyntax;
typedef SyntaxNodePlainList<DesignatorSyntax*> DesignatorListSyntax;
typedef SyntaxNodeSeparatedList<InitializerSyntax*> InitializerListSyntax;
typedef SyntaxNodeSeparatedList<ExpressionSyntax*> ExpressionListSyntax;
typedef SyntaxNodeSeparatedList<GenericAssociationSyntax*> GenericAssociationListSyntax;
typedef SyntaxNodePlainList<StatementSyntax*> StatementListSyntax;
typedef SyntaxNodeSeparatedList<ExtGNU_AsmOperandSyntax*> ExtGNU_AsmOperandListSyntax;

//=================================================================== Names

class DeclarationNameVisitor;
class DeclarationName;
class IdentifierName;
class Identifier;
class AnonymousName;
class TagName;

//=================================================================== Symbols

class SymbolTable;
class SymbolVisitor;

class Symbol;
class ScopeSymbol;
class VariableSymbol;
class ParameterSymbol;
class FunctionSymbol;
class BlockSymbol;
class StructOrUnionSymbol;
class EnumSymbol;
class EnumeratorDeclaration;
class ForwardDeclarationSymbol;

/* Types */
class TypeSymbol;
class UndefinedType;
class VoidType;
class IntegerType;
class FloatType;
class PointerType;
class ArrayType;
class NamedType;
class QuantifiedType;
class TypeVisitor;

/* Lists */
template <class PtrT> class SymbolList;

} // C
} // psy

#endif
